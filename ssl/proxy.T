
// -*-c++-*-
/* $Id: ok.T 3066 2007-09-19 00:36:58Z max $ */

#include "async.h"
#include "tame.h"
#include "tame_connectors.h"
#include "tame_io.h"
#include "parseopt.h"
#include "pubutil.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

//=======================================================================

BIO *_bio_err;

static bool
init_ssl_internals ()
{
  if (!_bio_err) {
    SSL_library_init ();
    SSL_load_error_strings ();
    _bio_err = BIO_new_fp (stderr, BIO_NOCLOSE);
  }
  return true;
}

static void 
ssl_complain (const str &s)
{
  if (s)
    BIO_printf (_bio_err, "%s", s.cstr ());
  ERR_print_errors (_bio_err);
  fflush (stderr);
}

static void
ssl_abort (const str &s)
{
  ssl_complain (s);
  exit (2);
}

//=======================================================================

class base_proxy_t  : public tame::std_proxy_t {
public:
  base_proxy_t (SSL *ssl, const str &d = NULL, ssize_t sz = -1)
    : tame::std_proxy_t (d, sz), 
      _ssl (ssl),
      _other_way (NULL) {}
  
  virtual ~base_proxy_t () {}
  void set_other_way (base_proxy_t *x) { _other_way = x; }

  virtual void force_write () { panic ("not implemented!\n"); }
  virtual void force_read ()  { panic ("not implemented!\n"); }

  void force_eof () { _eof = true; poke (); }

  void set_eof () 
  {
    if (!_eof) {
      _eof = true;
      _other_way->force_eof ();
    }
  }
    
protected:
  SSL *_ssl;
  base_proxy_t *_other_way;

};

//=======================================================================

class std_to_ssl_proxy_t : public base_proxy_t {
public:

  std_to_ssl_proxy_t (SSL *ssl, ssize_t sz = -1)
    : base_proxy_t (ssl, "std->ssl", sz),
      _force_write (false) {}

  ~std_to_ssl_proxy_t () {}

  void force_write () { _force_write = true; poke (); }

protected:
  bool is_writable () const;
  int v_write (int fd);

  bool _force_write;
};

//=======================================================================

class ssl_to_std_proxy_t : public base_proxy_t {
public:
  ssl_to_std_proxy_t (SSL *ssl, ssize_t sz = -1)
    : base_proxy_t (ssl, "ssl->std", sz),
      _force_read (false) {}
  
  ~ssl_to_std_proxy_t () {}

  void force_read () { _force_read = true; poke (); }

protected:
  bool is_readable () const;
  int v_read (int fd);

  bool _force_read;
};

//=======================================================================


bool
ssl_to_std_proxy_t::is_readable () const
{
  return (room_left () > 0 || _force_read);
}

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

class proxy_t {
public:
  proxy_t () : 
    _port (4433), 
    _fd (-1), 
    _dst_port (80), 
    _ssl_ctx (NULL) {}
  
  void run (int argc, char **argv, evv_t ev, CLOSURE);

private:
  bool parseconfig (int argc, char **argv);
  bool setup_sock ();
  void listen_loop (evv_t ev, CLOSURE);
  void serve (int fd, const sockaddr_in &sin, evv_t ev, CLOSURE);
  bool init_ssl ();
  void connect_to_dest (evi_t ev, CLOSURE);
  bool load_certificate ();
  bool init_ssl_connection (int s, SSL *s);

  int _port;
  int _fd;

  str _dst_hn;
  int _dst_port;

  str _certfile;

  SSL_CTX *_ssl_ctx;
};

static bool ssl_ok (int rc) { return rc == 1; }


static void
usage ()
{
  warnx << "usage: " << progname << " [-p <port>] -c <cert> <hostname>\n";
  exit (2);
}

bool
proxy_t::load_certificate ()
{
  bool ret = false;
  if (!ssl_ok (SSL_CTX_use_certificate_chain_file (_ssl_ctx, 
						   _certfile.cstr ()))) {
    ssl_complain ("use_certifcate() failed\n");
  } else if (!ssl_ok (SSL_CTX_use_PrivateKey_file (_ssl_ctx, 
						   _certfile.cstr (), 
						   SSL_FILETYPE_PEM))) {
    ssl_complain("use_PrivateKey() failed\n");
  } else {
    ret = true;
  }
  return ret;
}

bool
proxy_t::init_ssl ()
{
  SSL_METHOD *meth;
  bool ret = false;

  if (!init_ssl_internals ()) {
    warn << "Could never even initialize SSL error engine\n";
  } else if (!(meth = SSLv23_server_method ())) {
    warn << "Could not allocate SSL method\n";
  } else if (!(_ssl_ctx = SSL_CTX_new (meth))) {
    warn << "Cannot make new SSL context\n";
  } else if (!load_certificate ()) {
    warn << "Failed to load certificate.\n";
  } else {
    SSL_CTX_set_quiet_shutdown (_ssl_ctx, 1);
    SSL_CTX_sess_set_cache_size(_ssl_ctx, 0x1000);
    ret = true;
  }
  if (!ret) 
    ssl_abort ("init_ssl() failed\n");
  return ret;
}

bool
proxy_t::parseconfig (int argc, char **argv)
{
  int ch = -1;
  bool rc = true;
  while ((ch = getopt (argc, argv, "c:p:")) != -1 && rc) {
    switch (ch) {
    case 'c':
      _certfile = optarg;
      break;
    case 'p':
      if (!convertint (optarg, &_port)) {
	warn << "Cannot convertint: " << optarg << "\n";
	rc = false;
      }
      break;
    default:
      usage ();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 1 || 
      !(to_hostname_and_port (argv[0], &_dst_hn, &_dst_port)))
    usage ();

  if (!_certfile) {
    warn << "You must specify a certfile (.pem) with -c\n";
    usage ();
  }

  return rc;
}

bool
proxy_t::setup_sock ()
{
  u_int32_t listenaddr = INADDR_ANY;
  int fd = inetsocket (SOCK_STREAM, _port, listenaddr);
  if (fd < 0) {
    warn ("could not bind TCP port %d: %m\n", _port);
  } else {
    _fd = fd;
    listen (_fd, 5);
    warn << "listening on port " << _port << "\n";
  }
  return (fd >= 0);
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    proxy_t proxy;
  }

  twait { proxy.run (argc, argv, mkevent ()); }
  exit (0);
}

typedef enum { INTERRUPT, READ, EXIT } ev_t; 

tamed void
proxy_t::connect_to_dest (evi_t ev)
{
  tvars {
    int fd;
  }

  twait {
    tcpconnect (_dst_hn, _dst_port, mkevent (fd));
  }
  ev->trigger (fd);
}

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

tamed void
proxy_t::serve (int clifd, const sockaddr_in &sin, evv_t ev)
{
  tvars {
    SSL *ssl;
    int dstfd;
    ptr<base_proxy_t> prx[2];
  }

  make_async (clifd);

  if (!(ssl = SSL_new (_ssl_ctx))) {
    ssl_complain ("Failed to allocate a new SSL object\n");
  } else if (!init_ssl_connection (clifd, ssl)) {
    /* noop */
  } else {
    twait { connect_to_dest (mkevent (dstfd)); }
    if (dstfd < 0) {
      warn << "Failed to connect to host: " << _dst_hn 
	   << ":" << _dst_port << "\n";
    } else {
      prx[0] = New refcounted<ssl_to_std_proxy_t> (ssl);
      prx[1] = New refcounted<std_to_ssl_proxy_t> (ssl);

      for (size_t i = 0; i < 2; i++) {
	prx[i]->set_debug_level (2);
	prx[i]->set_other_way (prx[1 - i]);
      }
	
      twait {
	prx[0]->go (clifd, dstfd, mkevent ());
	prx[1]->go (dstfd, clifd, mkevent ());
      }

      // can only close FD after both proxies have stopped.
      close (dstfd);
    }
  }
  close (clifd);
  ev->trigger ();
}

tamed void
proxy_t::listen_loop (evv_t ev)
{
  tvars {
    bool go (true);
    sockaddr_in sin;
    socklen_t sinlen (sizeof (sin));
    rendezvous_t<ev_t> rv (__FILE__, __LINE__);
    int n_active (0);
    ev_t which;
    int nfd;
  }

  tame::sigcb1 (SIGINT,  mkevent (rv, INTERRUPT)); 
  tame::sigcb1 (SIGTERM, mkevent (rv, INTERRUPT));
  
  while (go || n_active > 0) {

    if (go) tame::waitread (_fd, mkevent (rv, READ));
    twait (rv, which);

    switch (which) {
    case INTERRUPT:
      warn << "Interrupt signal received; flushing remaining clients...\n";
      tame::clearread (_fd);
      close (_fd);
      go = false;
      break;
    case READ:
      bzero (&sin, sinlen);
      if ((nfd = accept (_fd, reinterpret_cast<sockaddr *> (&sin),
			 &sinlen)) >= 0) {
	warn ("acception connection from %s\n", inet_ntoa (sin.sin_addr));
	n_active ++;
	serve (nfd, sin, mkevent (rv, EXIT));
      } else {
	warn ("accept failed: %m\n");
      }
    case EXIT:
      n_active --;
      break;
    }
  }
  warn << "Exiting listen loop....\n";
  rv.cancel ();

  ev->trigger ();
}

tamed void
proxy_t::run (int argc, char **argv, evv_t ev)
{
  if (!parseconfig (argc, argv)) {
    /* noop */
  } else if (!setup_sock ()) {
    warn << "Socket setup failed\n";
  } else if (!init_ssl ()) {
    warn << "SSL initialization failed\n";
  } else {
    twait { listen_loop (mkevent ()); }
  }
  ev->trigger ();
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
  return 0;
}
