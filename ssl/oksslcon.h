
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

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace okssl {

  //-----------------------------------------------------------------------

  class con_t : public virtual refcount {
  public:
    con_t (int fd, SSL *ssl);
    ~con_t ();
    bool ok () const { return _ok; }
    void drain_to_network (strbuf *b, evb_t ev, CLOSURE);
    int _fd;
  private:
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

};

#endif /* HAVE_SSL */
#endif /* __SSL_OKSSLCON_H__ */
