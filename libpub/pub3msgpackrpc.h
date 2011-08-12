// -*-c++-*-

#pragma once

#include "async.h"
#include "pub3.h"

namespace pub3 {

  namespace msgpack {

    enum {
      REQUEST  = 0, //  [0, msgid, method, param]
      RESPONSE = 1, //  [1, msgid, error, result]
      NOTIFY = 2   //  [2, method, param]
    };

    //========================================

    typedef event<int, ptr<expr_t> >::ref ev_t;
    typedef event<int, ptr<expr_t>, ptr<expr_t> >::ref callev_t;

    //========================================

    class dependent_t {
    public:
      virtual void eof_hook (int rc) = 0;
      list_entry<dependent_t> _lnk;
    };

    //========================================

    class axprt : public virtual refcount {
    private:
      int _errno;
      int _fd;
      strbuf _inbuf;
      u_int32_t _seqid;

      qhash<u_int32_t, callev_t::ptr> _calls;

      axprt (int fd);
      tame::lock_t _write_lock, _read_lock;
      void close (int en);
      void recv_json (size_t lim, size_t ps, ev_t ev, CLOSURE);
      bool is_open () const { return _fd >= 0; }
      str get_str (size_t bytes) const;
      friend class refcounted<axprt>;

      u_int32_t seqid () ;

    public:
      static ptr<axprt> alloc (int fd);
      void send (ptr<const expr_t> x, evi_t ev, CLOSURE);
      void recv (ev_t ev, CLOSURE);
      void dispatch (CLOSURE);
      void call (str mthd, ptr<const expr_t> arg, callev_t ev, CLOSURE);
      
      ~axprt ();
    };
    
    //========================================

    class aclnt : public virtual refcount {
    private:
      ptr<axprt> _x;
      str _prog;
      aclnt (ptr<axprt> x, str prog);
      ~aclnt ();
      friend class refcounted<aclnt>;

    public:
      void call (str method, ptr<const expr_t> arg, ptr<expr_t> *res,
		 aclnt_cb cb, CLOSURE);
      static ptr<aclnt> alloc (ptr<axprt> x, str prog);
    };

    //========================================
    
  }

};
