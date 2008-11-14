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
      _ok (true),
      _connected (false)
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
      tame::iofd_t wfd (_self->_fd, selwrite);
      tame::iofd_t rfd (_self->_fd, selread);
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
	    twait { rfd.on (connector::cnc (mkevent (), ev, &oc)); }
	    go = (oc == OUTCOME_SUCC);
	  }
	  break;
	case SSL_ERROR_WANT_WRITE:
	  {
	    twait { wfd.on (connector::cnc (mkevent (), ev, &oc)); }
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
      tame::iofd_t rfd (_self->_fd, selread);
      tame::iofd_t wfd (_self->_fd, selwrite);
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

    twait { ssl_connect (connector::cnc (mkevent (go), ev)); }

    while (go) {
      if (need_write) {
	twait { wfd.on (mkevent ()); }
	SSL_write (_ssl, buf, 0);
      } else if (len > 0) {
	if (wait_for_read) {
	  twait { rfd.on (mkevent ()); }
	}
	ret = SSL_read (_ssl, out, len);

	{
	  if (ret > 0) {
	    str s ((const char *)out, ret);
	    warn << "read OK: len=" << ret<< "; dat=" << s << "\n";
	  } else {
	    warn << "read BAD: " << ret << "\n";
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

  tamed void
  asrc_t::init_T (cbv abuf_cb)
  {
    tvars {
      iovec *iov;
      ssize_t rc;
      ptr<bool> destroyed (_self->_destroyed);
      u_int32_t me (++_self->_current);
    }

    warn << me << ":init_T A\n";

    // By convention, call once if there's anything to read, of if
    // we're at the EOF.
    if (_uio.resid () || _eof) {
      (*abuf_cb) ();
    }


    //
    // What's _current == me?
    //
    //    Due to legacy designs, an abuf can be called init() upon
    //    multiple times -- for instance, for the cookie parsing,
    //    inside the header parsing.  In fact, that's really the only
    //    time.  In this case, the last call to this->init() should
    //    trample all else before.  When the cookie is doing being
    //    parse, then init() will be called again, in the original
    //    context.
    //
    //    In other words, _current == me ensures that only the most
    //    recent call to init() is going forward; the others should
    //    be dead.
    //
    while (_go && _current == me) {

      iov = _uio.get_iov ();
      warn << me << ":init_T B\n";
      twait { _con->read (iov[0].iov_base, iov[0].iov_len, mkevent (rc)); }
      warn << me << ":init_T C\n";
      
      if (*destroyed) 
	return;
      
      if (rc > 0) {
	_uio.account_for_new_bytes (rc);
      }
      if (rc == 0) {
	_eof = true;
	_go = false;
      }

      warn << me << ":init_T D\n";
      (*abuf_cb) ();
      warn << me << ":init_T E\n";
    }
    warn << me << ":init_T F\n";
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
