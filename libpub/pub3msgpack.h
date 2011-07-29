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
      outbuf_t ();
      str to_str () const { return _b; }
      void put_byte (u_int8_t b);
      void put_bytes (u_int8_t *b, size_t n);
      void put_str (str s);

      template<class T> void
      put_int (T i) 
      {
	size_t n = sizeof (T);
	size_t bits_per_byte = 8;
	size_t shift = (n - 1) * bits_per_byte;

	for (size_t j = 0; j < n; j++) {
	  u_int8_t b = ((i >> shift) & 0xff);
	  put_byte (b);
	  shift -= bits_per_byte;
	}
      }

      void encode_negative_int (int64_t i);
      void encode_positive_int (u_int64_t i);
      void encode_str (str s);

    private:
      void flush ();

      strbuf _b;
      size_t _tlen;
      mstr _tmp;
      char *_tp, *_ep;
    };

    //========================================

  };

};
