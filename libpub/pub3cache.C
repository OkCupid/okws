#include "pub3cache.h"

namespace pub3 {

  //=======================================================================

  bool cache_key_t::operator== (const cache_key_t &k) const
  { return _fn == k._fn && _opts == k._opts; }

  //-----------------------------------------------------------------------

  // mask in only those options that matter for indexing.
  u_int cache_key_t::op_mask (u_int in) { return in & (P_NOPARSE); }

  //-----------------------------------------------------------------------

  hash_t 
  cache_key_t::hash_me () const {
    strbuf b (_fn);
    b << ":" << _opts;
    str s = b;
    return hash_string (s.cstr ());
  }

  //=======================================================================

  ptr<file_lookup_t> 
  file_lookup_t::alloc () { return New refcounted<file_lookup_t> (); }

  //=======================================================================

};
