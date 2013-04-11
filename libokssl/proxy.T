// -*- mode: c++; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: nil; -*-
/* $Id: ok.T 3066 2007-09-19 00:36:58Z max $ */

#include "okwsconf.h"
#ifdef HAVE_SSL

#include "oksslproxy.h"
#include "oksslutil.h"
#include "okdbg.h"

//=======================================================================

namespace okssl {
  
  //-----------------------------------------------------------------------
  
  bool
  ssl_to_std_proxy_t::is_readable () const
  {
    return (room_left () > 0 || _force_read);
  }

  bool
  ssl_to_std_proxy_t::is_sync_readable () const
  {
      //  http://www.opensubscriber.com/message/openssl-users@openssl.org/8645223.html
      return (is_readable()
              && SSL_is_init_finished(_ssl)
              // Don't call ssl_pending during the handshake...
              && SSL_pending(_ssl) > 0);
  }

  //-----------------------------------------------------------------------
  
  void 
  base_proxy_t::set_eof () 
  {
    if (!_eof) {
      _eof = true;
      if (_other_way) _other_way->force_eof (); 
    }
  }
  
  //-----------------------------------------------------------------------
  
  int
  ssl_to_std_proxy_t::v_read (int fd) 
  {
#define BUFSZ 0x1000
    char buf[BUFSZ];
    int rc;
    int bytes_read = 0;
    bool doing_accept;
    do {
        if (!SSL_is_init_finished (_ssl)) {
            rc = SSL_accept (_ssl);
            doing_accept = true;
        } else {

            if (_handshake_ev) {
                evv_t::ptr ev = _handshake_ev;
                _handshake_ev = NULL;
                ev->trigger ();
            }

            ssize_t sz = room_left ();
            if (sz == 0 && !_force_read) {
                return bytes_read;
            }
            rc = SSL_read (_ssl, buf, min<ssize_t> (sz, BUFSZ));

            // MM: Don't let clients renegotiate if the option is set to 
            // prevent a DoS threat
            if (!_cli_renog && _renegotiations > 1) {
                ssl_complain ("SSL_read: error: SSL client renegotiation\n");
                return 0;
            }

            if (_force_read)
                _force_read = false;
            doing_accept = false;
        }

        if (rc > 0) {
            bytes_read += rc;
            if (!doing_accept)
                _buf.copy (buf, rc);
        } else if (rc < 0) {
            int err = SSL_get_error (_ssl, rc);
            switch (err) {
            case SSL_ERROR_WANT_READ:
                _force_read = true;
                errno = EAGAIN;
                break;
            case SSL_ERROR_WANT_WRITE:
                _other_way->force_write ();
                errno = EAGAIN;
                break;
            default:
                ssl_complain ("SSL_read encountered an error: ");
                errno = EIO;
                break;
            }
            return rc;
        }
    } while (rc > 0);

#undef BUFSZ
    return bytes_read;
  }

  //-----------------------------------------------------------------------

  bool 
  std_to_ssl_proxy_t::is_writable () const
  { 
    return (_buf.resid () > 0 || _force_write);
  }

  //-----------------------------------------------------------------------

  int
  std_to_ssl_proxy_t::v_write (int fd)
  {
    int rc = 1;
    assert (is_writable ());
    int nb = 0;

    while (rc > 0) {

      if (_buf.iovcnt () > 0) {
	rc = SSL_write (_ssl, _buf.iov ()[0].iov_base, _buf.iov ()[0].iov_len);
      } else {
	rc = SSL_write (_ssl, NULL, 0);
      }

      if (rc > 0) {
	nb += rc;
	_buf.rembytes (rc);
      } else if (rc < 0) {
	int err = SSL_get_error (_ssl, rc);
	switch (err) {
	case SSL_ERROR_WANT_READ:
	  _other_way->force_read ();
	  errno = EAGAIN;
	  break;
	case SSL_ERROR_WANT_WRITE:
	  _force_write = false;
	  errno = EAGAIN;
	  break;
	default:
	  ssl_complain ("SSL_write encountered an error: ");
	  errno = EIO;
	  break;
	}
      }
    }

    if (rc >= 0) rc = nb;
    return rc;
  }


  //-----------------------------------------------------------------------

  bool
  proxy_t::init_ssl_connection (int s, SSL *ssl)
  {
    bool ret = true;
    unsigned long sl = 1;
    if (BIO_socket_ioctl (s, FIONBIO, &sl) < 0) {
      ssl_complain ("ioctl failed on socket\n");
      ret = false;
    } else {
      // This bio is freed when we call SSL_free(ssl);
      BIO *sbio = BIO_new_socket (s, BIO_NOCLOSE);
      SSL_set_bio (ssl, sbio, sbio);
      SSL_set_accept_state (ssl);

      // XXX - untested
      SSL_set_mode (ssl, (SSL_MODE_ENABLE_PARTIAL_WRITE |
			  SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  proxy_t::init (SSL_CTX *ctx, int encfd, int plainfd, bool cli_renog)
  {
    bool ret = false;
    if (!ctx) {
      warn << "NULL ctx given\n";
    } else if (encfd < 0) {
      warn << "Invalid SSL-encoded FD given\n";
    } else if (plainfd < 0) {
      warn << "Invalid plaintext FD given\n";
    } else {
      _encfd = encfd;
      _plainfd = plainfd;
      _ssl = SSL_new (ctx);
      if (!_ssl) {
	warn << "Failed to allocate new SSL object!\n";
      } else if (!init_ssl_connection (encfd, _ssl)) {
	warn << "Failed to initalized SSL on given FD\n";
      } else {

	_handshaker = New refcounted<ssl_to_std_proxy_t>(_ssl, cli_renog);
	_prx[0] = _handshaker;
	_prx[1] = New refcounted<std_to_ssl_proxy_t> (_ssl);

	// MM: Set app data so we can track this thing
	ssl_to_std_proxy_t* phs = _handshaker;
	SSL_set_app_data(_ssl, phs);

	for (size_t i = 0; i < 2; i++) {
	  if (OKDBG2(SSL_PROXY)) {
	    _prx[i]->set_debug_level (2);
	  }
	  _prx[i]->set_other_way (_prx[1 - i]);
	}
	ret = true;
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  proxy_t::start (evb_t ev)
  {
    tvars {
      proxy_event_t which;
    }

    _handshaker->set_handshake_ev (mkevent (_rv, HANDSHAKE));
    _prx[0]->go (_encfd, _plainfd, mkevent (_rv, COMPLETE_A));
    _prx[1]->go (_plainfd, _encfd, mkevent (_rv, COMPLETE_B));

    twait (_rv, which);
    ev->trigger (which == HANDSHAKE);
  }

  //-----------------------------------------------------------------------

  tamed void
  proxy_t::finish (evv_t ev)
  {
    tvars {
      proxy_event_t which (NONE);
      size_t i;
    }
    
    // Someone should call this function to wait on the proxy to finish up.
    // If that waiter times out, we must make the assumption that the waiter
    // is going to close up its file descriptors.  Therefore, we MUST 
    // proactively tell the proxies to stop their proxying, so they know
    // in particular to stop selecting on file descriptors (and also to 
    // clean up memory, etc). 
    //
    // To implement this policy, we simply wait for the calling process
    // to cancel his waiting on us; when that happens, we'll cancel
    // waiting on the proxies, which will clean up their state.
    ev->set_cancel_notifier (mkevent (_rv, CANCELATION));

    // Keep waiting until the caller cancelled, or we successfully waited twice.
    for (i = 0; which != CANCELATION && i < 2; i++) {
      twait (_rv, which); 
    }

    // Even in the case that the proxy completed, we still need to cancel
    // this rendezvous, since we're no longer waiting on the cancel notifier
    // above.
    _rv.cancel ();

    // Can freely trigger even if the event was cancelled.
    ev->trigger (); 
  }

  //-----------------------------------------------------------------------
  
  base_proxy_t::base_proxy_t (SSL *ssl, const str &d, ssize_t sz)
    : tame::std_proxy_t (d, sz),
      _ssl (ssl),
      _other_way (NULL)
  {
    OKDBG4(SSL_MEM, CHATTER, "+ base_proxy_t %p\n", this);
  }

  //-----------------------------------------------------------------------

  base_proxy_t::~base_proxy_t ()
  {
    OKDBG4(SSL_MEM, CHATTER, "- base_proxy_t %p\n", this);
  }

  //-----------------------------------------------------------------------

  proxy_t::proxy_t (u_int dl)
    : _ssl (NULL),
      _encfd (-1),
      _plainfd (-1),
      _rv (__FILE__, __LINE__),
      _canceled (false)
  {
    OKDBG4(SSL_MEM, CHATTER, "+ proxy %p\n", this);
  }

  //-----------------------------------------------------------------------

  proxy_t::~proxy_t ()
  {
    OKDBG4(SSL_MEM, CHATTER, "- proxy %p\n", this);
    if (_encfd >= 0) close (_encfd);
    if (_encfd >= 0) close (_plainfd);
    if (_ssl) SSL_free (_ssl);
    if (_handshaker) { _handshaker = NULL; }
  }

  //-----------------------------------------------------------------------

  str
  proxy_t::cipher_info () const
  {
    // Older versions of SSL don't take a const SSL_CIPHER
    // for get_bits and get_version, etc.  So we need to use
    // a non-const cipher to work around that issue.
    const SSL_CIPHER *cipher;
    strbuf b;
    
    if (_ssl && (cipher = SSL_get_current_cipher(_ssl))) {
      const char *n = SSL_CIPHER_get_name (cipher);
      if (n) b << n;
      b << "--";
      int bits;
      if (SSL_CIPHER_get_bits (cipher, &bits) != 0)
	b << bits;
      b << "--";
      const char *v = SSL_CIPHER_get_version (cipher);
      b << v;
    }
    return b;
  }

  //-----------------------------------------------------------------------

  void
  proxy_t::cancel ()
  {
    if (!_canceled) {
      _canceled = true;
      _rv.cancel ();
    }
  }

  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
