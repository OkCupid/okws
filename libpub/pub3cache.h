// -*-c++-*-

#pragma once

#include "async.h"
#include "pub3eval.h"
#include "pub3parse.h"
#include "pub3file.h"
#include "timehash.h"

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

//-----------------------------------------------------------------------

template<> struct keyfn<pub3::cached_getfile_t, pub3::cache_key_t> {
  keyfn () {}
  const pub3::cache_key_t &operator() (const pub3::cached_getfile_t *o) 
    const { return o->_key; }
};

//-----------------------------------------------------------------------

namespace pub3 {

  //=======================================================================

  typedef timehash_t<str,str> negcache_t;
  typedef timehash_t<cache_key_t, cached_getfile_t> getfile_cache_t;

  //=======================================================================

  /**
   * A global lookup class for looking up cached pubfile
   * attributes.  See pub/pubd2.h for an example of a useful
   * global_t.
   *
   */
  class file_lookup_t {
  public:
    file_lookup_t () {}
    virtual ~file_lookup_t () {}

    static ptr<file_lookup_t> alloc ();

    virtual bool lookup (str nm, ptr<fhash_t> *hsh, time_t *ctime) 
    { return false; }

    virtual void cache_lookup (str j, str r, ptr<fhash_t> hsh, time_t ctime,
			       off_t sz) {}

    virtual bool getfile (ptr<fhash_t> h, opts_t opts, ptr<file_t> *f, 
			  parse_status_t *s, str *em) { return false; }

    virtual void cache_getfile (ptr<fhash_t> h, opts_t opts, ptr<file_t> f, 
				parse_status_t s, str em) {}

    virtual int hold_chunks (ptr<file_t> p) { return -1; }
    virtual ptr<file_t> get_chunks (ptr<fhash_t> h, opts_t opts) 
    { return NULL; }
  };

  //=======================================================================
};
