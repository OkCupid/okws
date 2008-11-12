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

  factory_t::~factory_t ()
  {
  }

  //-----------------------------------------------------------------------

#define BUFSZ 1024

  tamed void
  con_t::drain_to_network (strbuf *bp, evb_t ev)
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
      struct iovec iov;
      char buf[BUFSZ];
      int err;
    }

    while (go) {

      if (need_read) {
	twait { rfd.on (mkevent ()); }
	SSL_read (_ssl, buf, BUFSZ);
      } else if (uio->resid () > 0) {
	if (wait_for_write) {
	  twait { wfd.on (mkevent ()); }
	}
	iov = uio->iov ()[0];
	rc = SSL_write (_ssl, iov.iov_base, iov.iov_len);
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

#undef BUFSZ

  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
