
// -*-c++-*-

#ifndef __SSL_OKSSL_H__
#define __SSL_OKSSL_H__

#include "oksslproxy.h"

#if 0
class okssl {
  
  class manager_t {
  public:
    manager_t () : _ssl_ctx (NULL) {}
  
    void run (int argc, char **argv, evv_t ev, CLOSURE);

    void init (const str &certfile, int okfd, evb_t ev, CLOSURE);

  private:
    bool init_cert (const str &certfile);
    void init_okd_connection (int fd, evb_t ev, CLOSURE);


    bool parseconfig (int argc, char **argv);
    bool setup_sock ();
    void listen_loop (evv_t ev, CLOSURE);
    void serve (int fd, const sockaddr_in &sin, evv_t ev, CLOSURE);
    bool init_ssl ();
    void connect_to_dest (evi_t ev, CLOSURE);
    bool load_certificate ();

    str _certfile;
    ptr<axprt_unix> _okd_x;
    ptr<aclnt> _okd_cli;
    ptr<asrv> _okd_srv;

    int _port;
    int _fd;

    str _dst_hn;
    int _dst_port;


    SSL_CTX *_ssl_ctx;
  };
  
};
#endif

#endif /* __SSL_OKSSL_H__ */
