// -*-c++-*-

#pragma once

#include "async.h"
#include "pub3.h"

namespace pub3 {

  namespace msgpack {

    //========================================

    typedef event<int, ptr<expr_t> >::ref ev_t;

    class axprt : public virtual refcount {
    private:
      int _errno;
      int _fd;
      strbuf _inbuf;
      axprt (int fd);
      tame::lock_t _write_lock, _read_lock;
      void close (int en);
      void recv_json (size_t lim, size_t ps, ev_t ev, CLOSURE);
      bool is_open () const { return _fd >= 0; }
      str get_str (size_t bytes) const;
      friend class refcounted<axprt>;
    public:
      static ptr<axprt> alloc (int fd);
      void send (ptr<const expr_t> x, evi_t ev, CLOSURE);
      void recv (ev_t ev, CLOSURE);
      ~axprt ();
    };
    
    //========================================
    
  }

};
