// -*-c++-*-

#pragma once

#include "async.h"
#include "pub3pub.h"
#include "pub3file.h"

namespace pub3 {

  //=======================================================================

  /**
   * Keys for caching published files, both based on their filename,
   * and the desired publishing options; for instance, we need
   * different copies for WSS and non-WSS files.
   */
  struct cache_key_t {
    cache_key_t (str n, u_int o) 
      : _fn (n), _opts (op_mask (o)), _hsh (hash_me ()) {}

    const str &fn () const { return _fn; }
    u_int opts () const { return _opts; }

    const str _fn;
    const u_int _opts;
    const hash_t _hsh;

    bool operator== (const cache_key_t &k) const;
    operator hash_t () const { return _hsh; }

  private:
    // mask in only those options that matter for indexing.
    static u_int op_mask (u_int in) ;
    hash_t hash_me () const;
  };

  //=======================================================================

  /**
   * A getfile request that has been cached is stored here.
   */
  struct cached_getfile_t {
    cached_getfile_t (const cache_key_t &k, ptr<file_t> f)
      : _key (k), _file (f) {}

    const cache_key_t _key;
    ptr<file_t> file () const { return _file; }
    ptr<file_t> _file;
  };

  //=======================================================================

};
