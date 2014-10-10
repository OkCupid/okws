
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
    void drain_to_network (ptr<strbuf> b, evb_t ev) 
    { drain_to_network_T (b, ev); }
    void read (void *out, size_t len, evssz_t ev, CLOSURE);
    void drain_cancel (void);
    abuf_src_t *alloc_abuf_src ();
    bool ateof () const { return _eof; }
    ptr<tame::rcfd_t> _fd;
    void cancel() { _cancelled = true; }
  protected:
    void ssl_connect (evb_t ev, CLOSURE);
    void ssl_connect_2 (evb_t ev, CLOSURE);
    void drain_to_network_T (ptr<strbuf> b, evb_t ev, CLOSURE);
  private:
    SSL *_ssl;
    BIO *_rbio, *_wbio;
    bool _ok;
    bool _connected;
    ptr<tame::iofd_t> _rfd, _wfd;
    bool _eof;
    bool _cancelled;
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

  class asrc_t : public abuf_src_t {
  public:
    asrc_t (ptr<con_t> c);
    ~asrc_t () ;
    void init (cbv c);
    abuf_indata_t getdata ();
    void rembytes (int n);
    void finish () ;
    void cancel () ;
  protected:
    void read_loop (CLOSURE);
  private:
    ptr<con_t> _con;
    bool _go;
    bool _eof;
    suiolite _uio;
    bool _running;
    cbv::ptr _abuf_cb;
  public:
    ptr<bool> _destroyed;
    u_int32_t _current;
  };

  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
#endif /* __SSL_OKSSLCON_H__ */
