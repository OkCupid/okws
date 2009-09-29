// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3prot.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class fhash_t {
  public:
    fhash_t () {}
    fhash_t (const char *s) { memcpy (val, s, PUBHASHSIZE); }
    fhash_t (const xpub3_hash_t &h) { memcpy (val, h.base (), PUBHASHSIZE); }
    static ptr<fhash_t> alloc (const xpub3_hash_t &x) 
    { return New refcounted<fhash_t> (x); }
    str to_str () const { return armor64 (val, PUBHASHSIZE); }
    hash_t hash_hash () const;
    bool operator== (const fhash_t &ph) const;
    bool operator== (const xpub3_hash_t &ph) const;
    bool operator!= (const xpub3_hash_t &ph) const;
    bool operator!= (const fhash_t &ph) const;
    void to_xdr (xpub3_hash_t *ph) const;
    char *buffer () { return val; }

  private:
    char val[PUBHASHSIZE];
  };

  //-----------------------------------------------------------------------

  struct metadata_t : public okdbg_dumpable_t { 
    metadata_t () : _toplev (false) {}
    virtual ~metadata_t () {}
    metadata_t (const str &f, ptr<const fhash_t> h, bool tl = false) 
      : _jfn (f), _hsh (h), _toplev (tl) {}
    metadata_t (const xpub3_metadata_t &x);

    str jailed_filename () const;
    str real_filename () const;
    
    void to_xdr (xpub3_metadata_t *x) const;
    
    inline fhash_t hash () const { return hsh; }
    inline str filename () const { return fn; }

    bool operator== (const fhash_t &ph) { return *hsh == ph; }
    bool operator!= (const fhash_t &ph) { return !(*hsh == ph); }
    
    void okdbg_dump_vec (vec<str> *s) const;
    
    str _jfn;
    mutable str _rfn;

    ptr<const fhash_t> _hsh;
    bool _toplev;
  };

  //-----------------------------------------------------------------------

  class file_t {
  public:
    file_t (ptr<metadata_t> m, ptr<zone_t> z) : _metadata (m), _data_root (z) {}
    static ptr<file_t> alloc (ptr<metadata_t> m, ptr<zone_t> z);
    file_t (const xpub3_file_t &x);
    void to_xdr (xpub3_file_t *x) const;

  protected:
    ptr<metadata_t> _metadata;
    ptr<zone_t> _data_root;
  };

  //-----------------------------------------------------------------------

  ptr<fhash_t> file2hash (const str &fn, struct stat *sbp = NULL);
  bool file2hash (const str &fn, fhash_t *h, struct stat *sbp);

  //-----------------------------------------------------------------------

};

template<> struct hashfn<pub3::fhash_t> {
  hashfn () {}
  hash_t operator() (const pub3::fhash_t *s) const { return s->hash_hash (); } 
};

template<> struct equals<pub3::fhash_t> {
  equals () {}
  bool operator() (const pub3::fhash_t &s1, const pub3::fhash_t &s2) const
  { return (*s1 == *s2); }
};

