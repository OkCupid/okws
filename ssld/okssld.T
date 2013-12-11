
// -*-c++-*-

#include "okssl.h"
#include "oksslproxy.h"
#include "pub.h"
#include "okconst.h"
#include "okprot.h"
#include "ok.h"
#include "ahutil.h"
#include "oksslutil.h"
#include "tame_connectors.h"
#include "tame_io.h"

#ifdef HAVE_LINUX_PRCTL_DUMP
# include <sys/prctl.h>
#endif /* HAVE_LINUX_PRCTL_DUMP */

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
    void to_xdr (okssl_sendcon_arg_t *x);
  private:
    int _fd;
    sockaddr_in _sin;
    int _renegotiations;
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

    okssld_t () 
      : _logfd (-1), 
	_okd_fd (-1), 
	_accept_enabled (false),
	_timeout (ok_ssl_timeout),
	_debug_startup (false),
	_logd (NULL),
	_cipher_order(false),
	_cli_renog(false)
    {}

    bool parseopt (int argc, char *argv[]);

    void init (evb_t ev, CLOSURE);
    void run ();

    void handle_new_connection (port_t *p, ptr<con_t> con, CLOSURE);
    bool enable_accept ();
    bool disable_accept ();
    void dispatch (svccb *sbp);

  private:
    void okld_eof ();
    bool init_perms ();
    bool init_ports ();
    bool init_signals ();
    bool init_ssl ();
    bool init_okld ();
    void init_okd (evb_t ev, CLOSURE);
    void init_logd (evb_t ev, CLOSURE);
    bool load_certificate ();
    bool init_ciphers ();
    ok_xstatus_typ_t toggle_accept (bool b);
    void shutdown ();
    void sendcon (ptr<con_t> c, int fd, const port_t &p,
		  const str &cipher, evb_t ev, CLOSURE);
    void sig_ignore (int i);

    vec<port_t> _ports;

    int _uid, _gid;
    str _dir, _jaildir;
    str _certfile, _keyfile, _chainfile;
    int _logfd, _okd_fd;
    str _cipher_list;

    ptr<axprt_unix> _okd_x;
    ptr<aclnt> _okd_cli;
    ptr<asrv>  _okd_srv;
    ptr<aclnt> _okld_cli;

    SSL_CTX *_ssl_ctx;
    bool _accept_enabled;
    u_int _timeout;
    bool _debug_startup;
    fast_log_t *_logd;
    bool _cipher_order;
    bool _cli_renog;
  };

  //-----------------------------------------------------------------------

  void 
  con_t::to_xdr (okssl_sendcon_arg_t *x)
  {
    x->sin.setsize (sizeof (_sin));
    memcpy (x->sin.base (), &_sin, sizeof (_sin));
  }

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

  ok_xstatus_typ_t
  okssld_t::toggle_accept (bool b)
  {
    bool rc = b ? enable_accept () : disable_accept ();
    return (rc ? OK_STATUS_OK : OK_STATUS_ERR);
  }

  //-----------------------------------------------------------------------

  void
  okssld_t::shutdown ()
  {
    if (_logd) { _logd->flush (); }
    exit (0);
  }

  //-----------------------------------------------------------------------

  void
  okssld_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      warn << "Received EOF from okd; shutting down...\n";
      shutdown ();
      return;
    }
    switch (sbp->proc ()) {
    case OKSSL_TOGGLE_ACCEPT:
      {
	RPC::okssl_program_1::okssl_toggle_accept_srv_t<svccb> srv (sbp);
	ok_xstatus_typ_t rc = toggle_accept (*srv.getarg ());
	srv.reply (rc);
      }
      break;
    default:
      sbp->reject (PROC_UNAVAIL);
      break;
    }
  }

  //-----------------------------------------------------------------------

  tamed void
  okssld_t::sendcon (ptr<con_t> c, int fd, const port_t &p,
		     const str &cipher, evb_t ev)
  {
    tvars {
      str cli;
      okssl_sendcon_arg_t arg;
      ok_xstatus_typ_t res;
      clnt_stat err;
      bool ok (false);
    }
    
    c->to_xdr (&arg);
    arg.ssl.cipher = cipher;
    arg.port = p.port ();

    // don't have SFS libs close the FD (by passing false);
    // we close the FD explicitly below....
    _okd_x->sendfd (fd, false);

    twait {
      RPC::okssl_program_1::okssl_new_connection 
	(_okd_cli, arg, &res, mkevent (err));
    }

    if (err) {
      warn << "Failed to send connection (" << cli 
	   << ") to okd: " << err << "\n";
    } else if (res != OK_STATUS_OK) {
      warn ("okd rejected connection %s with status %d\n",
	    cli.cstr (), int (res));
    } else {
      ok = true;
    }

    close (fd);

    ev->trigger (ok);
  }

  //-----------------------------------------------------------------------

  tamed void
  okssld_t::handle_new_connection (port_t *p, ptr<con_t> c)
  {
    tvars {
      int fds[2];
      proxy_t prx;
      bool ok (false);
      outcome_t o;
      str cli;
      str msg ("-");
      str cipher ("-");
    }

    if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
      warn ("Cannot allocate a socketpair %m\n");
    } else {
      make_async (fds[0]);
      if (!prx.init (_ssl_ctx, c->fd (), fds[0], _cli_renog)) {
	warn ("Failed to initialize new proxy object\n");
      } else {
	cli = c->to_str ();
	twait { 
	  prx.start (connector::timeout (mkevent (ok), _timeout, 0, &o)); 
	}
	if (!ok) {
	  if (o != OUTCOME_SUCC) {
	    msg = "timeout in handshake";
	  } else {
	    msg = "handshake failed";
	  }
	  if (_logd) _logd->log_ssl (cli, "-", msg);
	  warn << "Error in handshake: " << msg << " for " << cli << "\n";
	  close (fds[1]);
	} else {
	  cipher = prx.cipher_info ();
	  twait { sendcon (c, fds[1], *p, prx.cipher_info (), mkevent (ok)); }
	  if (ok) {
	    twait { 
	      prx.finish (connector::timeout (mkevent (), _timeout, 0, &o)); 
	    }
	    if (o != OUTCOME_SUCC) {
	      msg = "SSL timeout in transmission";
	      warn ("Timeout in SSL transmission for client %s\n", cli.cstr ());
	      ok = false;
	    }
	  }
	}
	if (!ok) 
	  prx.cancel ();
	if (_logd)  {
	  _logd->log_ssl (cli, cipher, msg);
	}
      }
      close (fds[0]);
    }
  }

  //-----------------------------------------------------------------------

  bool
  con_t::accept (int fd)
  {
    socklen_t sinlen = sizeof (sockaddr_in);
    bzero (&_sin, sinlen);
    _fd = ::accept (fd, reinterpret_cast<sockaddr *> (&_sin), &sinlen);
    return (_fd >= 0);
  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::enable_accept ()
  {
    bool rc = !_accept_enabled;
    if (rc) {
      for (size_t i = 0; i < _ports.size (); i++) {
	_ports[i].enable ();
      }
    }
    return rc;
  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::disable_accept ()
  {
    bool rc = _accept_enabled;
    if (rc) {
      for (size_t i = 0; i < _ports.size (); i++) {
	_ports[i].disable ();
      }
    }
    return rc;
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
      if (!_dir) {
	warn ("No coredump/run directory specified! Cannot continue.\n");
      } else if (!_jaildir) {
	warn ("No jail directory speicified! Cannot continue.\n");
      } else if (chroot (_jaildir.cstr ()) != 0) {
	warn ("Could not chroot into run directory %s: %m\n", _jaildir.cstr ());
      } else if (setgid (_gid) != 0) {
	warn ("Cannot change group to GID=%d: %m\n", _gid);
      } else if (setuid (_uid) != 0) {
	warn ("Cannot change user to UID=%d: %m\n", _uid);
      } else if (chdir (_dir.cstr ()) != 0) {
	warn ("Cannot change into cored dump dir %s: %m\n", _dir.cstr ());
      } else {
	ret = true;
      }
    } else {
      if (!_dir) {
	warn ("Cannot find a run directory\n");
      } else if (chdir (_dir.cstr ()) != 0) {
	warn ("Could not change into run directory %s: %m\n", _dir.cstr ());
      } else {
	ret = true;
      }
    }

#ifdef HAVE_LINUX_PRCTL_DUMP
    if (prctl (PR_SET_DUMPABLE, 1) != 0) {
        warn ("prctl(PR_SET_DUMPABLE, 1) failed: %m\n");
    }
#endif /* HAVE_LINUX_PRCTL_DUMP */

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
    if (!_certfile && !_chainfile) {
      warn << "Exepcted either a CertChainFile or a CertFile!!\n";
    } else if (_certfile &&
	       !ssl_ok (SSL_CTX_use_certificate_file 
			(_ssl_ctx, _certfile.cstr (), SSL_FILETYPE_PEM))) {
      ssl_complain ("use_certifcate() failed\n");
    } else if (_chainfile && 
	       !ssl_ok (SSL_CTX_use_certificate_chain_file 
			(_ssl_ctx, _chainfile.cstr ()))) {
      ssl_complain ("use_certificate_chain_file() failed\n");

    } else if (!ssl_ok (SSL_CTX_use_PrivateKey_file (_ssl_ctx, 
						     _keyfile.cstr (), 
						     SSL_FILETYPE_PEM))) {
      ssl_complain("use_PrivateKey() failed\n");
    } else if (!ssl_ok (SSL_CTX_check_private_key (_ssl_ctx))) {
      ssl_complain("check private key failed\n");
    } else {
      if (_certfile) {
	warn << "Using cert: " << _certfile << "\n";
      }
      if (_chainfile) {
	warn << "Using cert chain: " << _chainfile << "\n";
      }
      warn << "Using key : " << _keyfile << "\n";
      ret = true;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  okssld_t::run ()
  {
    warn << "++ initialization complete; accepting connections\n";
    enable_accept ();
  }

  //-----------------------------------------------------------------------

  tamed void
  okssld_t::init_okd (evb_t ev)
  {
    tvars {
      clnt_stat err;
      bool rc (false);
    }

    if (_okd_fd < 0) {
      warn << "Invalid FD from okd acquired from okld: " << _okd_fd << "\n";
    } else {
      _okd_x = axprt_unix::alloc (_okd_fd);
      _okd_cli = aclnt::alloc (_okd_x, okssl_program_1);
      twait {
	RPC::okssl_program_1::okssl_null (_okd_cli, mkevent (err));
      }
      if (err) {
	warn << "Cannot ping okd: " << err << "\n";
      } else {
	_okd_srv = asrv::alloc (_okd_x, okssl_program_1,
				wrap (this, &okssld_t::dispatch));
	rc = true;
      }
    }
    ev->trigger (rc);
  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::init_ciphers ()
  {
    bool ret = true;
    if (_cipher_list && _cipher_list.len () &&
	SSL_CTX_set_cipher_list (_ssl_ctx, _cipher_list.cstr ()) != 1) {
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  static void ssl_info_callback(const SSL* ssl, int where, int ret) {
    if (0 != (where & SSL_CB_HANDSHAKE_START)) {
      ssl_to_std_proxy_t* prx = static_cast<ssl_to_std_proxy_t*> 
	SSL_get_app_data(ssl);
      if (prx) prx->renegotiate();
    } else if (0 != (where & SSL_CB_HANDSHAKE_DONE)) {
      ssl_to_std_proxy_t* prx = static_cast<ssl_to_std_proxy_t*> 
	SSL_get_app_data(ssl);
      if (prx && !prx->allow_cli_renog())
	ssl->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
    }
  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::init_ssl ()
  {
    OPENSSL_CONST SSL_METHOD *meth;
    bool ret = false;

    if (!init_ssl_internals ()) {
      warn << "Cannot initialize SSL engine internals\n";
    } else if (!_certfile && !_chainfile) {
      warn << "No certificate or certchain file specified\n";
    } else if (!_keyfile) {
      warn << "No private key file specified\n";
    } else if (!(meth = SSLv23_server_method ())) {
      warn << "Could not allocate SSL method\n";
    } else if (!(_ssl_ctx = SSL_CTX_new (meth))) {
      warn << "Cannot make new SSL context\n";
    } else if (!init_ciphers ()) {
      warn << "Cipher initialization failed\n";
    } else if (!load_certificate ()) {
      warn << "Failed to load certificate\n";
    } else {
      SSL_CTX_set_quiet_shutdown (_ssl_ctx, 1);
      SSL_CTX_sess_set_cache_size(_ssl_ctx, 0x1000);
#ifdef HAVE_SSL_NOCOMP 
      SSL_CTX_set_options(_ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif
      if (_cipher_order)
          SSL_CTX_set_options(_ssl_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);

      SSL_CTX_set_options(_ssl_ctx, SSL_OP_SINGLE_DH_USE);
      SSL_CTX_set_options(_ssl_ctx, SSL_OP_SINGLE_ECDH_USE);

      SSL_CTX_set_tmp_ecdh(_ssl_ctx, 
                           EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));

      SSL_CTX_set_info_callback(_ssl_ctx, ssl_info_callback);
      ret = true;
    }
    if (!ret) 
      ssl_complain ("init_ssl() failed\n");
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::init_okld ()
  {
    bool ret = false;
    ptr<axprt> x = axprt_unix::alloc (0);
    if ((_okld_cli = aclnt::alloc (x, null_program_1))) {
      _okld_cli->seteofcb (wrap (this, &okssld_t::okld_eof));
      ret = true;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  okssld_t::okld_eof ()
  {
    warn << "EOF from okld; shutting down!\n";
    shutdown ();
  }

  //-----------------------------------------------------------------------

  void
  okssld_t::sig_ignore (int i)
  {
    warn << "ignoring signal " << i << "\n";
  }

  //-----------------------------------------------------------------------

  bool
  okssld_t::init_signals ()
  {
    int sigs[] = { SIGUSR1, SIGUSR2, 0 };
    for (int *sp = sigs; *sp; sp++) {
      sigcb (*sp, wrap (this, &okssld_t::sig_ignore, *sp));
    }
    return true;
  }

  //-----------------------------------------------------------------------
  
  tamed void
  okssld_t::init (evb_t ev)
  {
    tvars {
      bool rc;
      bool okd_rc, logd_rc;
    }
    
    if (_debug_startup) {
      warn ("DEBUG: waiting for CONT before continuing (pid=%d)\n", getpid ());
      twait { tame::sigcb1 (SIGCONT, mkevent ()); }
    }

    rc = init_okld () && init_ssl () && init_ports () && init_perms ();

    if (rc) {
      twait { 
	init_okd (mkevent (okd_rc));
	init_logd (mkevent (logd_rc));
      }
    }

    ev->trigger (rc && okd_rc && logd_rc);
  }

  //-----------------------------------------------------------------------

  tamed void
  okssld_t::init_logd (evb_t ev)
  {
    tvars {
      bool ok (false);
    }

    if (_logfd < 0) {
      warn << "Invalid log FD given: " << _logfd << "\n";
    } else {
      _logd = New fast_log_t (_logfd, ok_access_log_fmt);
      twait { 
	_logd->connect (mkevent (ok));
      }
      if (!ok) {
	warn << "Failed to connect to logd\n";
      }
    }
    ev->trigger (ok);
  }

  //-----------------------------------------------------------------------
  
  bool
  okssld_t::parseopt (int argc, char *argv[])
  {
    int ch;
    bool rc = true;
    while (rc && (ch = getopt (argc, argv, "RODm:c:k:u:g:d:l:t:p:j:n:L:")) != -1) {
      switch (ch) {
      case 'j':
	_jaildir = optarg;
	break;
      case 'D':
	_debug_startup = true;
	break;
      case 'k':
	_keyfile = optarg;
	break;
      case 'c':
	_certfile = optarg;
	break;
      case 'n':
	_chainfile = optarg;
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
      case 'm':
	if (!convertint (optarg, &_timeout)) {
	  warn << "Cannot parse timeout " << optarg << "\n";
	  rc = false;
	}
	break;
      case 't':
	if (!convertint (optarg, &_okd_fd)) {
	  warn << "Cannot parse FD to okd " << optarg << "\n";
	  rc = false;
	}
	break;
      case 'L':
	_cipher_list = optarg;
	break;
      case 'O':
	_cipher_order = true;
	break;
      case 'R':
	_cli_renog = true;
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


tamed static void
main2 (int argc, char **argv)
{
  tvars {
    bool ok;
  }
  
  okssld = New okssl::okssld_t ();
  warn << "Starting up; pid=" << getpid () << "\n";

  if (!okssld->parseopt (argc, argv)) {
    warn << "Parse of okssl options failed\n";
    exit (-1);
  }

  twait { okssld->init (mkevent (ok)); }
  if (!ok) {
    warn << "Failed to initialize SSL\n";
    exit (-1);
  }

  okssld->run ();
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  set_debug_flags ();
  main2 (argc, argv);
  amain ();
  return 0;
}

//-----------------------------------------------------------------------

