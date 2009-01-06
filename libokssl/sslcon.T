// -*-c++-*-

#include "okwsconf.h"
#ifdef HAVE_SSL

#include "oksslcon.h"
#include "oksslutil.h"
#include "tame_io.h"
#include "okdbg.h"

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
      _ok (true),
      _connected (false),
      _rfd (fd, selread),
      _wfd (fd, selwrite)
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
  
  tamed void
  con_t::ssl_connect (evb_t ev)
  {
    tvars {
      bool rc;
    }

    if (_connected) {
      rc = true;
    } else {
      twait { ssl_connect_2 (connector::cnc (mkevent (rc), ev)); }
      if (!rc) {
	warn << "SSL connection establishment failed\n";
      }
    }
    ev->trigger (rc);

  }

  //-----------------------------------------------------------------------
  
#define BUFSZ 1024

  tamed void
  con_t::ssl_connect_2 (evb_t ev)
  {
    tvars {
      int err, rc;
      bool ret (false);
      bool go (true);
      outcome_t oc (OUTCOME_SUCC);
    }

    while (go) {
      go = false;

      rc = SSL_connect (_ssl);
      if (rc < 0) {
	err = SSL_get_error (_ssl, rc);
	switch (err) {
	case SSL_ERROR_WANT_READ:
	  {
	    twait { _rfd.on (connector::cnc (mkevent (), ev, &oc)); }
	    go = (oc == OUTCOME_SUCC);
	  }
	  break;
	case SSL_ERROR_WANT_WRITE:
	  {
	    twait { _wfd.on (connector::cnc (mkevent (), ev, &oc)); }
	    go = (oc == OUTCOME_SUCC);
	  }
	  break;
	default:
	  ssl_complain ("unhandled error durring connection: ");
	  errno = EIO;
	  break;
	}
      } else {
	ret = true;
	_connected = true;
      }
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------


  tamed void
  con_t::drain_to_network_T (strbuf *bp, evb_t ev)
  {
    tvars {
      suio *uio (bp->tosuio ());
      bool ret (true);
      bool go (true);
      int rc;
      bool need_read (false);
      bool wait_for_write (true);
      char buf[BUFSZ];
      int err;
      outcome_t oc;
    }

    twait { ssl_connect (connector::cnc (mkevent (go), ev)); }
    if (!go) { ret = false; }

    while (go) {

      if (need_read) {
	twait { _rfd.on (connector::cnc (mkevent (), ev, &oc)); }
	if (oc == OUTCOME_SUCC) { 
	  SSL_read (_ssl, buf, 0);
	  need_read = false;
	} else {
	  go = false;
	}
      } else if (uio->resid () > 0 && wait_for_write) {
	twait { _wfd.on (connector::cnc (mkevent (), ev, &oc)); }
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
      bool go (true);
      bool need_write (false);
      bool wait_for_read (false);
      ssize_t ret (-1);
      char buf[BUFSZ];
      int err;
    }

    twait { ssl_connect (connector::cnc (mkevent (go), ev)); }

    while (go) {
      if (need_write) {
	twait { _wfd.on (mkevent ()); }
	SSL_write (_ssl, buf, 0);
      } else if (len > 0) {
	if (wait_for_read) {
	  twait { _rfd.on (mkevent ()); }
	}
	ret = SSL_read (_ssl, out, len);

	if (OKDBG2 (SSL_INDATA)) {
	  if (ret > 0) {
	    str s (reinterpret_cast<const char *> (out), ret);
	    fprintf (stderr, "SSL read: %s\n", s.cstr ());
	  }
	}

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

  void
  asrc_t::init (cbv abuf_cb)
  {
    _abuf_cb = abuf_cb;

    // By convention, call once if there's anything to read, of if
    // we're at the EOF.
    if (_uio.resid () || _eof) {
      (*_abuf_cb) ();
    }

    read_loop ();
  }

  //-----------------------------------------------------------------------

  tamed void
  asrc_t::read_loop ()
  {
    tvars {
      iovec *iov;
      ssize_t rc;
      ptr<bool> destroyed (_self->_destroyed);
    }

    if (_running)
      return;
    _running = true;

    while (_go) {

      iov = _uio.get_iov ();
      twait { _con->read (iov[0].iov_base, iov[0].iov_len, mkevent (rc)); }
      
      if (*destroyed) 
	return;
      
      if (rc > 0) {
	_uio.account_for_new_bytes (rc);
      }
      if (rc == 0) {
	_eof = true;
	_go = false;
      }

      (*_abuf_cb) ();
    }
    _running = false;
  }
  
  //-----------------------------------------------------------------------

  abuf_indata_t
  asrc_t::getdata ()
  {
    if (_eof) return abuf_indata_t ();
    else return abuf_indata_t (&_uio);
  }

  //-----------------------------------------------------------------------

  asrc_t::asrc_t (ptr<con_t> c)
    : _con (c),
      _go (true),
      _eof (false),
      _uio (20000),
      _running (false),
      _destroyed (New refcounted<bool> (false)),
      _current (0) {}
      

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

  void
  asrc_t::rembytes (int n)
  {
    _uio.rembytes (n);
  }

  //-----------------------------------------------------------------------

  asrc_t::~asrc_t ()
  {
    *_destroyed = true;
  }

  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
