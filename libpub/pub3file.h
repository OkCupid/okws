// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3prot.h"
#include "pub3base.h"
#include "pub3expr.h"
#include "okdbg.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class zone_t;

  //-----------------------------------------------------------------------

  class fhash_t {
  public:
    fhash_t () {}
    fhash_t (const char *s) { memcpy (_b, s, PUBHASHSIZE); }
    fhash_t (const xpub3_hash_t &h) { memcpy (_b, h.base (), PUBHASHSIZE); }
    static ptr<fhash_t> alloc (const xpub3_hash_t &x) 
    { return New refcounted<fhash_t> (x); }
    str to_str () const { return armor64 (_b, PUBHASHSIZE); }
    str to_str_16 () const;
    hash_t hash_hash () const;
    bool operator== (const fhash_t &ph) const;
    bool operator== (const xpub3_hash_t &ph) const;
    bool operator!= (const xpub3_hash_t &ph) const;
    bool operator!= (const fhash_t &ph) const;
    void to_xdr (xpub3_hash_t *ph) const;
    char *buf () { return _b; }
    const char *buf () const { return _b; }

  private:
    char _b[PUBHASHSIZE];
  };

  //-----------------------------------------------------------------------

  struct metadata_t {
    metadata_t () : _toplev (false), _ctime (0) {}
    virtual ~metadata_t () {}

    metadata_t (const str &j, const str &r, ptr<const fhash_t> h, 
		bool tl = false) 
      : _jfn (j), _rfn (r), _hsh (h), _toplev (tl) , _ctime (0) {}

    metadata_t (const str &j, ptr<const fhash_t> h, bool tl = false) 
      : _jfn (j), _hsh (h), _toplev (tl) , _ctime (0) {}

    metadata_t (const xpub3_metadata_t &x);
    static ptr<metadata_t> alloc (const xpub3_metadata_t &x);

    str jailed_filename () const;
    str real_filename () const;
    void set_input_filename (str s) { _ifn = s; }
    
    void to_xdr (xpub3_metadata_t *x) const;
    
    inline fhash_t hash () const { return *_hsh; }
    ptr<const fhash_t> hashp () const { return _hsh; }
    inline str filename () const { return _jfn; }
    str input_filename () const { return _ifn; }
    ptr<expr_dict_t> to_dict () const;
    ptr<expr_dict_t> to_binding () const;

    bool operator== (const fhash_t &ph) { return _hsh && *_hsh == ph; }
    bool operator!= (const fhash_t &ph) { return !_hsh || !(*_hsh == ph); }
    
    void okdbg_dump_vec (vec<str> *s) const;

    void set_ctime (time_t t) { _ctime = t; }
    
    str _jfn;
    str _rfn;

    ptr<const fhash_t> _hsh;
    bool _toplev;
    time_t _ctime;
    str _ifn;  // input filename, before localization
  };

  //-----------------------------------------------------------------------

  class file_t {
  public:
    file_t (ptr<metadata_t> m, ptr<zone_t> z, opts_t o = 0) 
      : _metadata (m), _data_root (z), _opts (o) {}
    static ptr<file_t> alloc (const xpub3_file_t &file, opts_t o = 0);
    static ptr<file_t> alloc (ptr<metadata_t> m, ptr<zone_t> z, opts_t o = 0);
    static ptr<file_t> raw_alloc (ptr<metadata_t> m, zstr z, opts_t o = 0);
    file_t (const xpub3_file_t &x, opts_t o = 0);
    void to_xdr (xpub3_file_t *x) const;
    ptr<const metadata_t> metadata () const { return _metadata; }
    ptr<metadata_t> metadata () { return _metadata; }
    ptr<const zone_t> data () const { return _data_root; }

    // for chunked file xfer from pubd to services...
    void init_xdr_opaque ();
    ssize_t xdr_len () const;
    ssize_t get_chunk (size_t offset, char *buf, size_t capacity) const;
    void get_xdr_hash (xpub3_hash_t *x) const;
    opts_t opts () const { return _opts; }

  protected:
    ptr<metadata_t> _metadata;
    ptr<zone_t> _data_root;
    opts_t _opts;

    str _xdr_opaque;
    fhash_t _xdr_opaque_hash;
  };

  //-----------------------------------------------------------------------

  ptr<fhash_t> file2hash (const str &fn, struct stat *sbp = NULL);
  bool file2hash (const str &fn, fhash_t *h, struct stat *sbp);

  //-----------------------------------------------------------------------

  typedef enum { JAIL_NONE = 0, JAIL_VIRTUAL = 1, 
		 JAIL_REAL = 2, JAIL_PERMISSIVE = 3 } jail_mode_t;

  //-----------------------------------------------------------------------

  class jailer_t {
  public:
    jailer_t (jail_mode_t m = JAIL_NONE, str d = NULL);
    void set (jail_mode_t m = JAIL_NONE, str d = NULL);
    str jail2real (str s) const;
    static ptr<jailer_t> alloc ();
    static void set_in_jail (bool b);
    void setjail (jail_mode_t m, str d);
  protected:
    static bool be_verbose ();
    static bool in_jail ();
    static bool _in_jail;
    jail_mode_t _mode;
    str _dir;
  };

  //-----------------------------------------------------------------------

};

template<> struct hashfn<pub3::fhash_t> {
  hashfn () {}
  hash_t operator() (const pub3::fhash_t *s) const { return s->hash_hash (); } 
};

template<> struct equals<pub3::fhash_t> {
  equals () {}
  bool operator() (const pub3::fhash_t *s1, const pub3::fhash_t *s2) const
  { return (*s1 == *s2); }
};

