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

  factory_t::~factory_t ()
  {
  }

  //-----------------------------------------------------------------------

  void
  con_t::drain_cancel ()
  {
    if (_cancel_ev) {
      evv_t::ptr c = _cancel_ev;
      _cancel_ev = NULL;
      c->trigger ();
    }
  }

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
      rendezvous_t<bool> rv;
      bool do_wait;
    }

    assert (!_cancel_ev);
    _cancel_ev = mkevent (rv, false);

    while (go) {

      do_wait = true;
      if (need_read) {
	rfd.on (mkevent (rv, true));
      } else if (uio->resid () > 0 && wait_for_write) {
	wfd.on (mkevent (rv, true));
      } else {
	do_wait = false;
      }

      if (do_wait) 
	twait (rv, go);

      if (!go) {
	ret = false;
      } else if (need_read) {
	// read 0 bytes of data for now.
	SSL_read (_ssl, buf, 0);
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
    _cancel_ev = NULL;
    ev->trigger (ret);
  }


  //-----------------------------------------------------------------------

  tamed void
  con_t::read (char *out, size_t len, evssz_t ev)
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
    return New okssl_src_t (mkref (this));
  }
  
  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
