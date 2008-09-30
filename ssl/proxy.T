
// -*-c++-*-
/* $Id: ok.T 3066 2007-09-19 00:36:58Z max $ */

#include "oksslproxy.h"
#include "oksslutil.h"

//=======================================================================

namespace okssl {
  
  //-----------------------------------------------------------------------
  
  bool
  ssl_to_std_proxy_t::is_readable () const
  {
    return (room_left () > 0 || _force_read);
  }
  
  //-----------------------------------------------------------------------
  
  void 
  base_proxy_t::set_eof () 
  {
    if (!_eof) {
      _eof = true;
      _other_way->force_eof ();
    }
  }
  
  //-----------------------------------------------------------------------
  
  int
  ssl_to_std_proxy_t::v_read (int fd) 
  {
#define BUFSZ 0x1000
    char buf[BUFSZ];
    int rc;
    bool doing_accept;
    
    if (!SSL_is_init_finished (_ssl)) {
      rc = SSL_accept (_ssl);
      doing_accept = true;
    } else {
     
      if (_handshake_ev) {
	_handshake_ev = NULL;
	evv_t::ptr ev = _handshake_ev;
	ev->trigger ();
      }

      int sz = room_left ();
      assert (sz > 0 || _force_read);
      rc = SSL_read (_ssl, buf, sz);
      if (_force_read)
	_force_read = false;
      doing_accept = false;
    }
    
    if (rc > 0) {
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
    }

#undef BUFSZ
    return rc;
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
      BIO *sbio = BIO_new_socket (s, BIO_NOCLOSE);
      SSL_set_bio (ssl, sbio, sbio);
      SSL_set_accept_state (ssl);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  proxy_t::init (SSL_CTX *ctx, int encfd, int plainfd)
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

	_handshaker = New refcounted<ssl_to_std_proxy_t> (_ssl);
	_prx[0] = _handshaker;
	_prx[1] = New refcounted<std_to_ssl_proxy_t> (_ssl);
	
	for (size_t i = 0; i < 2; i++) {
	  _prx[i]->set_debug_level (2);
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
      bool ok (false);
      proxy_event_t which;
    }

    _handshaker->set_handshake_ev (mkevent (_rv, HANDSHAKE));
    _prx[0]->go (_encfd, _plainfd, mkevent (_rv, COMPLETE_A));
    _prx[1]->go (_plainfd, _encfd, mkevent (_rv, COMPLETE_B));

    twait (_rv, which);
    if (which == HANDSHAKE) {
      ok = true;
    } else {
      _rv.cancel ();
      _canceled = true;
    }

    ev->trigger (ok);
  }

  //-----------------------------------------------------------------------

  tamed void
  proxy_t::finish (evv_t ev)
  {
    tvars {
      proxy_event_t which;
    }
    twait (_rv, which);
    twait (_rv, which);
    ev->trigger ();
  }

  //-----------------------------------------------------------------------

  proxy_t::~proxy_t ()
  {
    if (_encfd >= 0) close (_encfd);
    if (_encfd >= 0) close (_plainfd);
    if (_ssl) SSL_free (_ssl);
  }

  //-----------------------------------------------------------------------

  str
  proxy_t::cipher_info () const
  {
    const SSL_CIPHER *cipher;
    strbuf b;
    
    if (_ssl && (cipher = SSL_get_current_cipher(_ssl))) {
      const char *n = SSL_CIPHER_get_name (cipher);
      if (n) b << n;
      b << "-";
      int bits;
      if (ssl_ok (SSL_CIPHER_get_bits (cipher, &bits)))
	b << bits;
      b << "-";
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
