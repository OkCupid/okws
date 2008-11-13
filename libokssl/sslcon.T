// -*-c++-*-

#include "okwsconf.h"
#ifdef HAVE_SSL

#include "oksslcon.h"
#include "oksslutil.h"
#include "tame_io.h"

namespace okssl {

  //-----------------------------------------------------------------------

  ptr<con_t>
  factory_t::alloc_con (int fd)
  { 
    return New refcounted<con_t> (fd, SSL_new (_ctx));
  } 

  //-----------------------------------------------------------------------

  con_t::con_t (int fd, SSL *ssl)
    : _fd (fd), 
      _ssl (ssl),
      _rbio (BIO_new_socket (fd, BIO_NOCLOSE)),
      _wbio (BIO_new_socket (fd, BIO_NOCLOSE)),
      _ok (true)
  {
    unsigned long sl = 1;
    if (BIO_socket_ioctl (fd, FIONBIO, &sl) < 0) {
      ssl_complain ("ioctl failed on socket for FIONBIO\n");
      _ok = false;
    } else {
      SSL_set_bio (_ssl, _rbio, _wbio);
      SSL_set_mode (_ssl, (SSL_MODE_ENABLE_PARTIAL_WRITE |
			  SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER));
    }
  }

  //-----------------------------------------------------------------------

  con_t::~con_t ()
  {
    if (_fd >= 0) {
      close (_fd);
      _fd = -1;
    }
    SSL_free (_ssl);
  }


  //-----------------------------------------------------------------------

  factory_t::factory_t ()
  {
    SSLeay_add_ssl_algorithms ();
    SSL_METHOD *meth = SSLv23_client_method ();
    SSL_load_error_strings ();
    _ctx = SSL_CTX_new (meth);
    SSL_CTX_set_default_verify_paths (_ctx);
    SSL_CTX_set_verify (_ctx, SSL_VERIFY_NONE, NULL);
  }

  //-----------------------------------------------------------------------

  static ptr<factory_t> g_factory;

  ptr<factory_t>
  factory ()
  {
    if (!g_factory)
      g_factory = New refcounted<factory_t> ();
    return g_factory;
  }

  //-----------------------------------------------------------------------

  factory_t::~factory_t () {} 
  //-----------------------------------------------------------------------

  void con_t::drain_cancel () {}

  //-----------------------------------------------------------------------

#define BUFSZ 1024

  tamed void
  con_t::drain_to_network_T (strbuf *bp, evb_t ev)
  {
    tvars {
      suio *uio (bp->tosuio ());
      bool ret (true);
      bool go (true);
      bool rc;
      tame::iofd_t rfd (_self->_fd, selread);
      tame::iofd_t wfd (_self->_fd, selwrite);
      bool need_read (false);
      bool wait_for_write (true);
      char buf[BUFSZ];
      int err;
      outcome_t oc;
    }

    while (go) {

      if (need_read) {
	twait { rfd.on (connector::cnc (mkevent (), ev, &oc)); }
	if (oc == OUTCOME_SUCC) { 
	  SSL_read (_ssl, buf, 0);
	  need_read = false;
	} else {
	  go = false;
	}
      } else if (uio->resid () > 0 && wait_for_write) {
	twait { wfd.on (connector::cnc (mkevent (), ev, &oc)); }
	wait_for_write = false;
	go = (oc == OUTCOME_SUCC);
      }

      if (!go) {
	ret = false;
      } else if (uio->resid () > 0) {

	{
	  const struct iovec &iov = uio->iov ()[0];
	  rc = SSL_write (_ssl, iov.iov_base, iov.iov_len);
	}

	if (rc >= 0) {
	  uio->rembytes (rc);
	} else {
	  err = SSL_get_error (_ssl, rc);
	  switch (err) {
	  case SSL_ERROR_WANT_READ:
	    need_read = true;
	    break;
	  case SSL_ERROR_WANT_WRITE:
	    wait_for_write = true;
	    break;
	  default:
	    ssl_complain ("SSL_write encountered an error: ");
	    errno = EIO;
	    go = false;
	    ret = false;
	    break;
	  }
	}
      } else {
	go = false;
      }
    }
    ev->trigger (ret);
  }


  //-----------------------------------------------------------------------

  tamed void
  con_t::read (void *out, size_t len, evssz_t ev)
  {
    tvars {
      tame::iofd_t rfd (_self->_fd, selread);
      tame::iofd_t wfd (_self->_fd, selwrite);
      bool go (true);
      bool need_write (false);
      bool wait_for_read (true);
      ssize_t ret (-1);
      char buf[BUFSZ];
      int err;
    }

    while (go) {
      if (need_write) {
	twait { wfd.on (mkevent ()); }
	SSL_write (_ssl, buf, 0);
      } else if (len > 0) {
	if (wait_for_read) {
	  twait { rfd.on (mkevent ()); }
	}
	ret = SSL_read (_ssl, out, len);
	if (ret < 0) {
	  err = SSL_get_error (_ssl, ret);
	  switch (err) {
	  case SSL_ERROR_WANT_READ:
	    wait_for_read = true;
	    break;
	  case SSL_ERROR_WANT_WRITE:
	    need_write = true;
	    break;
	  default:
	    ssl_complain ("SSL_write encountered an error: ");
	    errno = EIO;
	    go = false;
	    break;
	  }
	} else {
	  go = false;
	}
      }
    }
    ev->trigger (ret);
  }
#undef BUFSZ

  //-----------------------------------------------------------------------

  abuf_src_t *
  con_t::alloc_abuf_src ()
  {
    return New asrc_t (mkref (this));
  }
  
  //-----------------------------------------------------------------------
  
  tamed void
  asrc_t::init_T (cbv abuf_cb)
  {
    tvars {
      iovec *iov;
      ssize_t rc;
    }
    while (_go) {
      iov = _uio.get_iov ();
      twait { _con->read (iov[0].iov_base, iov[0].iov_len, mkevent (rc)); }
      if (rc > 0) {
	_uio.account_for_new_bytes (rc);
      }
      if (rc == 0) {
	_eof = true;
	_go = false;
      }
      (*abuf_cb) ();
    }
  }
  
  //-----------------------------------------------------------------------

  void
  asrc_t::readcb (int n)
  {

  }

  //-----------------------------------------------------------------------

  abuf_indata_t
  asrc_t::getdata ()
  {
    if (_eof) return abuf_indata_t ();
    else return abuf_indata_t (&_uio);
  }

  //-----------------------------------------------------------------------

  void
  asrc_t::finish ()
  {
    _go = false;
  }

  //-----------------------------------------------------------------------

  void
  asrc_t::cancel ()
  {
    _go = false;
  }

  //-----------------------------------------------------------------------

  bool
  asrc_t::overflow ()
  {
    // XXX implement me!
    return false;
  }

  //-----------------------------------------------------------------------

  void
  asrc_t::rembytes (int n)
  {
    _uio.rembytes (n);
  }

  //-----------------------------------------------------------------------

  asrc_t::~asrc_t ()
  {
    // XXX implement me!
  }

  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
