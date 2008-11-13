
// -*-c++-*-

#ifndef __SSL_OKSSLCON_H__
#define __SSL_OKSSLCON_H__

#include "okwsconf.h"
#ifdef HAVE_SSL

#include "async.h"
#include "tame.h"
#include "tame_connectors.h"
#include "tame_io.h"
#include "tame_rendezvous.h"
#include "abuf.h"

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace okssl {

  typedef event<ssize_t>::ref evssz_t;


  //-----------------------------------------------------------------------

  class con_t : public ok_xprt_base_t {
  public:
    con_t (int fd, SSL *ssl);
    ~con_t ();
    bool ok () const { return _ok; }
    void drain_to_network (strbuf *b, evb_t ev) { drain_to_network_T (b, ev); }
    void read (char *out, size_t len, evssz_t ev, CLOSURE);
    void drain_cancel (void);
    abuf_src_t *alloc_abuf_src ();
    int _fd;
  protected:
    void drain_to_network_T (strbuf *b, evb_t ev, CLOSURE);
  private:
    evv_t::ptr _cancel_ev;
    SSL *_ssl;
    BIO *_rbio, *_wbio;
    bool _ok;
  };

  //-----------------------------------------------------------------------

  class factory_t {
  public:
    factory_t ();
    ~factory_t ();
    ptr<con_t> alloc_con (int fd);
  private:
    SSL_CTX *_ctx;
  };

  ptr<factory_t> factory ();

  //-----------------------------------------------------------------------

  class okssl_src_t : public abuf_src_t {
  public:
    okssl_src_t (ptr<con_t> c) : _con (c) {}
    ~okssl_src_t () ;
    void init (cbv c);
    void readcb (int n);
    abuf_indata_t getdata ();
    void rembytes (int n);
    void finish () ;
    void cancel () ;
    bool overflow ();
  private:
    ptr<con_t> _con;
  };

  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
#endif /* __SSL_OKSSLCON_H__ */
