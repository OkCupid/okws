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
    fhash_t (const xpubhash_t &h) { memcpy (val, h.base (), PUBHASHSIZE); }
    static ptr<fhash_t> alloc (const xpubhash_t &x) 
    { return New refcounted<fhash_t> (x); }
    str to_str () const { return armor64 (val, PUBHASHSIZE); }
    hash_t hash_hash () const;
    bool operator== (const fhash_t &ph) const;
    bool operator== (const xpubhash_t &ph) const;
    bool operator!= (const xpubhash_t &ph) const;
    bool operator!= (const fhash_t &ph) const;
    void to_xdr (xpubhash_t *ph) const;

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

  protected:
    ptr<metadata_t> _metadata;
    ptr<zone_t> _data_root;
  };

  //-----------------------------------------------------------------------

};

