// -*-c++-*-

#include "okwsconf.h"
#ifdef HAVE_SSL

#include "oksslcon.h"
#include "oksslutil.h"
#include "tame_io.h"
#include "okdbg.h"
#include "okssl.h"

namespace okssl {

  ptr<con_t>
  factory_t::alloc_con (int fd)
  {
    return New refcounted<con_t> (fd, SSL_new (_ctx));
  }

  con_t::con_t (int fd, SSL *ssl)
    : _fd (tame::rcfd_t::alloc (fd)),
      _ssl (ssl),
      _rbio (BIO_new_socket (fd, BIO_NOCLOSE)),
      _wbio (BIO_new_socket (fd, BIO_NOCLOSE)),
      _ok (true),
      _connected (false),
      _rfd (tame::iofd_t::alloc (_fd, selread)),
      _wfd (tame::iofd_t::alloc (_fd, selwrite)),
      _eof (false),
      _cancelled (false)
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


  con_t::~con_t ()
  {
    SSL_free (_ssl);
    _ssl = NULL;
  }

  factory_t::factory_t ()
  {
    // gets overwritten in okssld
    SSLeay_add_ssl_algorithms ();
    OPENSSL_CONST SSL_METHOD *meth = SSLv23_client_method ();
    SSL_load_error_strings ();
    _ctx = SSL_CTX_new (meth);
    SSL_CTX_set_default_verify_paths (_ctx);
    SSL_CTX_set_options(_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    SSL_CTX_set_verify (_ctx, SSL_VERIFY_NONE, NULL);
  }


  static ptr<factory_t> g_factory;

  ptr<factory_t>
  factory ()
  {
    if (!g_factory)
      g_factory = New refcounted<factory_t> ();
    return g_factory;
  }


  factory_t::~factory_t () {} 


  void con_t::drain_cancel () {}

  
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
	    twait { _rfd->on (connector::cnc (mkevent (), ev, &oc)); }
	    go = (oc == OUTCOME_SUCC);
	  }
	  break;
	case SSL_ERROR_WANT_WRITE:
	  {
	    twait { _wfd->on (connector::cnc (mkevent (), ev, &oc)); }
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


  tamed void
  con_t::drain_to_network_T (ptr<strbuf> bp, evb_t ev)
  {
    tvars {
      suio *uio (bp->tosuio ());
      bool ret (true);
      bool go (false);
      int rc;
      bool need_read (false);
      bool wait_for_write (true);
      char buf[BUFSZ];
      int err;
      outcome_t oc;

      // This is overkill, but in case there's a bug, we won't scribble
      // over memory this way.
      holdvar ptr<con_t> hold (mkref (_self));
    }

    // Key to set go=false, in case we're canceled while trying
    // to establish an SSL connection.  In that case, we want
    // to early-out of this function!
    twait { ssl_connect (connector::cnc (mkevent (go), ev)); }
    if (!go) { ret = false; }

    while (go) {

      if (need_read) {
	twait { _rfd->on (connector::cnc (mkevent (), ev, &oc)); }
	if (oc == OUTCOME_SUCC) { 
	  SSL_read (_ssl, buf, 0);
	  need_read = false;
	} else {
	  go = false;
	}
      } else if (uio->resid () > 0 && wait_for_write) {
	twait { _wfd->on (connector::cnc (mkevent (), ev, &oc)); }
	wait_for_write = false;
	go = (oc == OUTCOME_SUCC);
      }

      if (!go) {
	ret = false;
      } else if (uio->resid () > 0) {

	{
	  const struct iovec &iov = uio->iov ()[0];
	  rc = SSL_write (_ssl, iov.iov_base, iov.iov_len);

	  if (OKDBG2 (SSL_OUTDATA)) {
	    if (rc > 0) {
	      strbuf b;
	      str s (reinterpret_cast<const char *> (iov.iov_base), rc);
	      b << "++ SSL write (" << rc << "," << iov.iov_len << "): "
		<< s << "\n";
	      b.tosuio ()->output (2);
	    }
	  }
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
    if (OKDBG2 (SSL_OUTDATA)) {
      size_t sz = uio->resid ();
      okdbg_warn (CHATTER, "leaving sslcon::drain; rc=%d; resid=%zu\n",
		  ret, sz);
    }
    ev->trigger (ret);
  }



  tamed void
  con_t::read (void *out, size_t len, evssz_t ev)
  {
    tvars {
      bool go (false);
      bool need_write (false);
      bool wait_for_read (false);
      ssize_t ret (-1);
      char buf[BUFSZ];
      int err;
      size_t tot (0);
      outcome_t oc (OUTCOME_SUCC);
    }

    twait { ssl_connect (connector::cnc (mkevent (go), ev, &oc)); }

    // Just for sanity checking, don't go if we were cancelled.
    if (oc != OUTCOME_SUCC) { assert (!go); }

    // We loop for a read, not because we're going to do multiple reads,
    // but because we might need to write before we can read.  Also,
    // only bother to enter this loop if there is data to read and room
    // in the buffer.  I'm not sure why we'd be called with a len=0, but
    // guard against it here.
    while (go && len > 0 && !_cancelled) {

      go = false;

      if (need_write) {
	twait { _wfd->on (connector::cnc (mkevent (), ev, &oc)); }

	// We always need to check that the outcome is not a cancel.
	// In the case of cancel we shouldn't touch the buffer we're
	// reading to ("out");
	if (oc == OUTCOME_SUCC) {
	  SSL_write (_ssl, buf, 0);
	  go = true; // it's worth another trip through the loop!
	}

      } else {

	// This should always stay so!
	// Anyways, now we need to read something from the network.
	assert (len > 0);

	// In certain error cases, the SSL socket told us to wait 
	// so in this case, we do just that.
	if (wait_for_read) {
	  twait { _rfd->on (connector::cnc (mkevent (), ev, &oc)); }
	}

	// Again, protect all forward progress by worrying about 
	// cancelation by our caller.  If we were canceled, then
	// don't do anything further.
	if (oc == OUTCOME_SUCC && !_cancelled) {

	  // This is the actual read.
	  ret = SSL_read (_ssl, out, len);

	  if (ret == 0) { 
	    // On EOF, we're done reading, and nothing doin' here
	    _eof = true; 

	  } else if (ret > 0) { 
	    tot += ret;  // keep this just for debugging.
	  
	    // Debug output... say what data is actually read.
	    if (OKDBG2 (SSL_INDATA)) {
	      strbuf b;
	      str s (reinterpret_cast<const char *> (out), ret);
	      b << "++ SSL read: " << s << "\n";
	      b.tosuio ()->output (2);
	    }
	    
	  } else {

	    // Handle various errors -- a lot of them are retryable
	    // and allow us to keep going in our network I/O.
	    err = SSL_get_error (_ssl, ret);
	    switch (err) {
	    case SSL_ERROR_WANT_READ:
	      wait_for_read = true;
	      go = true;
	      break;
	    case SSL_ERROR_WANT_WRITE:
	      need_write = true;
	      go = true;
	      break;
	    default:
	      ssl_complain ("SSL_write encountered an error: ");
	      errno = EIO;
	      break;
	    }
	  }
	}
      }
    }

    OKDBG4 (SSL_INDATA, CHATTER, 
	    "leaving sslcon::read; rc=%zd; bytes=%zu\n", ret, tot);

    if (oc != OUTCOME_SUCC) {
      OKDBG3 (SSL_INDATA, CHATTER, "sslcon::read cancelled by caller");
    }

    ev->trigger (ret);
  }
#undef BUFSZ


  abuf_src_t *
  con_t::alloc_abuf_src ()
  {
    return New asrc_t (mkref (this));
  }
  

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


  tamed void
  asrc_t::read_loop ()
  {
    tvars {
      iovec *iov;
      ssize_t rc;
      ptr<bool> destroyed (_self->_destroyed);
      ptr<con_t> c;
    }

    // Hold onto con ourselves, since this object might have deallocated
    // out from underneath us. That's fine, we handle that case.  However,
    // _con ought to be kept around too.  So holding onto it here in the closure
    // does the trick!
    c = _con;

    if (_running)
      return;
    _running = true;

    while (_go) {

      iov = _uio.get_iov ();
      twait { c->read (iov[0].iov_base, iov[0].iov_len, mkevent (rc)); }

      if (*destroyed)
	return;

      if (rc > 0) {
	_uio.account_for_new_bytes (rc);
      } else if (rc == 0) {
	_eof = true;
	_go = false;
      } else if (rc < 0 && errno != EAGAIN) {
	warn ("okssl::asrc_t::read_loop failing: %m\n");
	_eof = true;
	_go = false;
      }

      (*_abuf_cb) ();

      // the above callback might have freed memory out from underneath
      // us (if everything got deallocated).  In that case, we gotta
      // jet out of here.
      if (*destroyed)
	return;
    }
    _running = false;
  }


  abuf_indata_t
  asrc_t::getdata ()
  {
    if (_eof) return abuf_indata_t ();
    else return abuf_indata_t (&_uio);
  }


  asrc_t::asrc_t (ptr<con_t> c)
    : _con (c),
      _go (true),
      _eof (false),
      _uio (20000),
      _running (false),
      _destroyed (New refcounted<bool> (false)),
      _current (0) {}



  void
  asrc_t::finish ()
  {
    _go = false;
  }


  void
  asrc_t::cancel ()
  {
    _go = false;
  }


  void
  asrc_t::rembytes (int n)
  {
    _uio.rembytes (n);
  }


  asrc_t::~asrc_t ()
  {
      _con->cancel();
      *_destroyed = true;
  }


};

#endif /* HAVE_SSL */
