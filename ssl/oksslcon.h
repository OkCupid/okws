
// -*-c++-*-

#ifndef __SSL_OKSSLCON_H__
#define __SSL_OKSSLCON_H__
#ifdef HAVE_SSL

namespace okssl {

  //-----------------------------------------------------------------------

  class con_t {
  public:
    con_t (const str &hn, int port) : _hostname (hn), _port (port) {}
  private:
    str _hostname;
    int _port;
  };

  //-----------------------------------------------------------------------

  class factory_t {
  public:
    ptr<con_t> alloc_con (const str &hn, int port);
  };

  ptr<factory_t> factory ();

  //-----------------------------------------------------------------------

};

#endif /* HAVE_SSL */
#endif /* __SSL_OKSSLCON_H__ */
