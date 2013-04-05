
// -*-c++-*-

#pragma once

#include "okwsconf.h"
#ifdef HAVE_SSL

#include "async.h"
#include "tame.h"
#include "tame_connectors.h"
#include "tame_io.h"
#include "parseopt.h"
#include "pubutil.h"
#include "tame_rendezvous.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace okssl {

  //=======================================================================
  
  class base_proxy_t  : public tame::std_proxy_t {
  public:
    base_proxy_t (SSL *ssl, const str &d = NULL, ssize_t sz = -1);
    virtual ~base_proxy_t ();

    void set_other_way (base_proxy_t *x) { _other_way = x; }
    
    virtual void force_write () { panic ("not implemented!\n"); }
    virtual void force_read ()  { panic ("not implemented!\n"); }
    void force_eof () { _eof = true; poke (); }
    void set_eof () ;
    bool allow_unclean_shutdowns () const { return true; }
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
    ssl_to_std_proxy_t (SSL *ssl, bool cli_renog, ssize_t sz = -1)
      : base_proxy_t (ssl, "ssl->std", sz),
	_force_read (false), _renegotiations(0), 
	_cli_renog(cli_renog) {}
    
    ~ssl_to_std_proxy_t () {}
    void force_read () { _force_read = true; poke (); }
    void set_handshake_ev (evv_t::ptr ev) { _handshake_ev = ev; }

    void renegotiate() { _renegotiations++; }
    bool allow_cli_renog() const { return _cli_renog; }
    
  protected:
    bool is_readable () const;
    bool is_sync_readable () const;
    int v_read (int fd);
    bool _force_read;
    evv_t::ptr _handshake_ev;
    int _renegotiations;
    bool _cli_renog;
  };
  
  //=======================================================================

  typedef enum { NONE = 0,
		 HANDSHAKE = 1, 
		 COMPLETE_A = 2, 
		 COMPLETE_B = 3,
		 CANCELATION = 4 } proxy_event_t;

  class proxy_t {
  public:
    proxy_t (u_int debug_level = 0) ;
    ~proxy_t () ;
    bool init (SSL_CTX *ctx, int encfd, int plainfd, bool cli_renog);
    void start (evb_t ev, CLOSURE);
    void finish (evv_t ev, CLOSURE);
    str cipher_info () const;
    void cancel ();

  private:
    bool init_ssl_connection (int , SSL *);
    SSL *_ssl;
    int _encfd, _plainfd;
    ptr<base_proxy_t> _prx[2];
    ptr<ssl_to_std_proxy_t> _handshaker;
    rendezvous_t<proxy_event_t> _rv;
    bool _canceled;
    u_int _debug_level;
  };
  
  //=======================================================================

};


# endif /* HAVE_SSL */
