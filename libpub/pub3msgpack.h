// -*-c++-*-

#pragma once

#include "async.h"
#include "pub3.h"

namespace pub3 {

  namespace msgpack {

    //========================================

    ptr<pub3::expr_t> decode (str m);
    str encode (ptr<const pub3::expr_t> x);

    //========================================
    
    class outbuf_t {
    public:
      outbuf_t () {}
      void put_str (str s);
      str to_str () const { return _b; }
    private:
      strbuf _b;
      vec<str> _hold;
    };

    //========================================

  };

};
