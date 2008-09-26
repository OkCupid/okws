
// -*-c++-*-

#include "okssl.h"
#include "oksslproxy.h"

#if 0
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

	
	// can only close FD after both proxies have stopped.
	close (dstfd);
      }
    }
    close (clifd);
    ev->trigger ();
  }
 
  //-----------------------------------------------------------------------

  static bool ssl_ok (int rc) { return rc == 1; }
  
  //-----------------------------------------------------------------------

  static void
  usage ()
  {
    warnx << "usage: " << progname << " [-p <port>] -c <cert> <hostname>\n";
    exit (2);
  }

  //-----------------------------------------------------------------------

  static void
  ssl_abort (const str &s)
  {
    ssl_complain (s);
    exit (2);
  }

  //-----------------------------------------------------------------------
  
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
  
  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------


  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------

};
#endif

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  // main2 (argc, argv);
  amain ();
  return 0;
}


