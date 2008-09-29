
// -*-c++-*-

#include "okssl.h"
#include "oksslproxy.h"
#include "pub.h"
#include "okconst.h"
#include "okprot.h"
#include "ok.h"
#include "ahutil.h"
#include "oksslutil.h"

//-----------------------------------------------------------------------

namespace okssl {

  //-----------------------------------------------------------------------

  class okssld_t;

  //-----------------------------------------------------------------------

  class con_t {
  public:
    con_t () {}
    bool accept (int fd);
    int fd () { return _fd; }
    str to_str () const;
  private:
    int _fd;
    sockaddr_in _sin;
  };

  //-----------------------------------------------------------------------

  class port_t {
  public:
    port_t (int p, okssld_t *s) : _port (p), _fd (-1), _okssl (s) {}
    bool init ();
    void enable ();
    void disable ();
    int port () const { return _port; }
  private:
    void accept_cb ();
    int _port;
    int _fd;
    okssld_t *_okssl;
  };

  //-----------------------------------------------------------------------
  
  class okssld_t {
  public:
    okssld_t () {}
    bool parseopt (int argc, char *argv[]);

    bool init ();
    void run ();

    void handle_new_connection (port_t *p, ptr<con_t> con, CLOSURE);
    void enable_accept ();
    void disable_accept ();

  private:
    bool init_perms ();
    bool init_ports ();
    bool init_ssl ();
    bool load_certificate ();

    vec<port_t> _ports;

    int _uid, _gid;
    str _dir;
    str _certfile;
    int _logfd, _okdfd;

    SSL_CTX *_ssl_ctx;
  };

  //-----------------------------------------------------------------------

  void
  port_t::enable ()
  {
    fdcb (_fd, selread, wrap (this, &port_t::accept_cb));
  }

  //-----------------------------------------------------------------------

  void
  port_t::disable ()
  {
    fdcb (_fd, selread, NULL);
  }

  //-----------------------------------------------------------------------

  void
  port_t::accept_cb ()
  {
    ptr<con_t> c = New refcounted<con_t> ();
    if (!c->accept (_fd)) {
      warn ("accept failure: %m\n");
    } else {
      _okssl->handle_new_connection (this, c);
    }
  }

  //-----------------------------------------------------------------------

  str
  con_t::to_str () const
  {
    const char *s = inet_ntoa (_sin.sin_addr);
    if (!s) {
      s = "0.0.0.0";
    }
    return s;
  }

  //-----------------------------------------------------------------------

  tamed void
  okssld_t::handle_new_connection (port_t *p, ptr<con_t> c)
  {
    tvars {
      int fds[2];
      proxy_t prx;
      str cli;
      bool ok;
      bool close0 (true), close1 (true);
    }

    if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
      warn ("Cannot allocate a socketpair %m\n");
    } else {
      make_async (fds[0]);
      if (!prx.init (_ssl_ctx, c->fd (), fds[0])) {
	warn ("Failed to initialize new proxy object\n");
      } else {
	cli = c->to_str ();
	twait { prx.start (mkevent (ok));}
	if (!ok) {
	  warn ("Failed to handshake with remote client %s\n", cli.cstr ());
	} else {
	}
      }
    }
    if (close0) close (fds[0]);
    if (close1) close (fds[1]);

  }

  //-----------------------------------------------------------------------

  bool
  con_t::accept (int fd)
  {
    size_t sinlen = sizeof (sockaddr_in);
    bzero (&_sin, sinlen);
    _fd = ::accept (fd, reinterpret_cast<sockaddr *> (&_sin), &sinlen);
    return (_fd >= 0);
  }

  //-----------------------------------------------------------------------

  void
  okssld_t::enable_accept ()
  {
    for (size_t i = 0; i < _ports.size (); i++) {
      _ports[i].enable ();
    }
  }

  //-----------------------------------------------------------------------

  void
  okssld_t::disable_accept ()
  {
    for (size_t i = 0; i < _ports.size (); i++) {
      _ports[i].disable ();
    }
  }

  //-----------------------------------------------------------------------

  bool
  port_t::init ()
  {
    u_int32_t listenaddr = INADDR_ANY; // XXX allow addr selection
    int fd = inetsocket (SOCK_STREAM, _port, listenaddr);
    if (fd < 0) {
      warn ("could not bind to TCP port %d: %m\n", _port);
    } else {
      _fd = fd;
      listen (_fd, 100); // XXX allow specification of listen Q size
      warn << "listening on port " << _port << "\n";
    }
    return (fd >= 0);
  }

  //-----------------------------------------------------------------------

  bool 
  okssld_t::init_perms ()
  {
    bool ret = false;
    if (!getuid ()) {
      if (chroot (_dir.cstr ()) != 0) {
	warn ("Could not chroot into run directory %s: %m\n", _dir.cstr ());
      } else if (setgid (_gid) != 0) {
	warn ("Cannot change group to GID=%d: %m\n", _gid);
      } else if (setuid (_uid) != 0) {
	warn ("Cannot change user to UID=%d: %m\n", _uid);
      } else {
	ret = true;
      }
    } else {
      if (chdir (_dir.cstr ()) != 0) {
	warn ("Could not change into run directory %s: %m\n", _dir.cstr ());
      } else {
	ret = true;
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::init_ports ()
  {
    bool rc = true;
    for (size_t i = 0; i < _ports.size (); i++) {
      if (!_ports[i].init ())
	rc = false;
    }
    return rc;
  }

  //-----------------------------------------------------------------------
  
  bool
  okssld_t::load_certificate ()
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

  //-----------------------------------------------------------------------

  void
  okssld_t::run ()
  {

  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::init_ssl ()
  {
    SSL_METHOD *meth;
    bool ret = false;

    if (!init_ssl_internals ()) {
      warn << "Cannot initialize SSL engine internals\n";
    } else if (!_certfile) {
      warn << "No certificate file specified\n";
    } else if (!(meth = SSLv23_server_method ())) {
      warn << "Could not allocate SSL method\n";
    } else if (!(_ssl_ctx = SSL_CTX_new (meth))) {
      warn << "Cannot make new SSL context\n";
    } else if (!load_certificate ()) {
      warn << "Failed to load certificate\n";
    } else {
      SSL_CTX_set_quiet_shutdown (_ssl_ctx, 1);
      SSL_CTX_sess_set_cache_size(_ssl_ctx, 0x1000);
      ret = true;
    }
    if (!ret) 
      ssl_complain ("init_ssl() failed\n");
    return ret;
  }

  //-----------------------------------------------------------------------
  
  bool
  okssld_t::init ()
  {
    return 
      init_ssl () &&
      init_ports () &&
      init_perms ();
  }

  //-----------------------------------------------------------------------
  
  bool
  okssld_t::parseopt (int argc, char *argv[])
  {
    int ch;
    bool rc = true;
    while (rc && (ch = getopt (argc, argv, "k:u:g:d:l:t:p:")) != -1) {
      switch (ch) {
      case 'k':
	_certfile = optarg;
	break;
      case 'u':
	if (!convertint (optarg, &_uid)) {
	  warn << "Cannot parse uid " << optarg << "\n";
	  rc = false;
	}
	break;
      case 'g':
	if (!convertint (optarg, &_gid)) {
	  warn << "Cannot parse gid " << optarg << "\n";
	  rc = false;
	}
	break;
      case 'd':
	_dir = optarg;
	break;
      case 'l':
	if (!convertint (optarg, &_logfd)) {
	  warn << "Cannot parse log FD " << optarg << "\n";
	  rc = false;
	}
	break;
      case 't':
	if (!convertint (optarg, &_okdfd)) {
	  warn << "Cannot parse FD to okd " << optarg << "\n";
	  rc = false;
	}
	break;
      case 'p':
	{
	  okws1_port_t port;
	  if (!convertint (optarg, &port)) {
	    warn << "Cannot parse port " << optarg << "\n";
	    rc = false;
	  } else {
	    _ports.push_back (port_t (port, this));
	  }
	  break;
	}
      default:
	warn << "Unrecognized option passed\n";
	break;
      }
    }
    return rc;
  }
  
};

//-----------------------------------------------------------------------

okssl::okssld_t *okssld;

//-----------------------------------------------------------------------


int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  okssld = New okssl::okssld_t ();

  if (!okssld->parseopt (argc, argv)) {
    warn << "Parse of okssl options failed\n";
    return -1;
  }

  if (!okssld->init ()) {
    warn << "Failed to initialize SSL\n";
    return -1;
  }

  okssld->run ();

  amain ();
  return 0;
}

//-----------------------------------------------------------------------

