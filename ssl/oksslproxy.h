
// -*-c++-*-

#ifndef __SSL_OKSSLPROXY_H__
#define __SSL_OKSSLPROXY_H__

#include "async.h"
#include "tame.h"
#include "tame_connectors.h"
#include "tame_io.h"
#include "parseopt.h"
#include "pubutil.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace okssl {

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
    void set_eof () ;
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

  class proxy_t {
  public:
    proxy_t () : _ssl (NULL), _encfd (-1), _plainfd (-1) {}
    ~proxy_t () ;
    bool init (SSL_CTX *ctx, int encfd, int plainfd);
    void run (evv_t ev, CLOSURE);
  private:
    bool init_ssl_connection (int s, SSL *s);
    SSL *_ssl;
    int _encfd, _plainfd;
    ptr<base_proxy_t> _prx[2];
  };

  //=======================================================================

};


#endif /* __SSL_OKSSL_PROXY_H_ */
