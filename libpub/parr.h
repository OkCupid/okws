
// -*-c++-*-
/* $Id$ */

#ifndef _LIBPUB_PARR_H
#define _LIBPUB_PARR_H

#include "pub.h"

#define INT64_MAX   0x7fffffffffffffffLL 
#define INT64_MIN (-0x7fffffffffffffffLL - 1)

class parr_int_t;
class parr_char_t;
class parr_int16_t;
class parr_int64_t;
class parr_uint_t;
class parr_uint16_t;

class parr_t : public pval_t {
public:
  parr_t () {}
  virtual ~parr_t () {}
  virtual bool add (int64_t i) = 0;
  virtual bool add (ptr<pval_t> v) { return false; }
  virtual u_int size () const = 0;
  const parr_t *to_arr () const { return this; }
};

class parr_ival_t : public parr_t {
public:
  parr_ival_t () {}
  virtual ~parr_ival_t () {}
  const parr_ival_t *to_int_arr () const { return this; }

  virtual ptr<parr_int_t> to_int () { return NULL; }
  virtual ptr<parr_char_t> to_char () { return NULL; }
  virtual ptr<parr_int16_t> to_int16 () { return NULL; }
  virtual ptr<parr_int64_t> to_int64 () { return NULL; }
  virtual ptr<parr_uint_t> to_uint () { return NULL; }
  virtual ptr<parr_uint16_t> to_uint16 () { return NULL; }

  virtual parr_err_t val (u_int i, int64_t *p) const { return PARR_BAD_TYPE; }
  virtual parr_err_t val (u_int i, int *p) const { return PARR_BAD_TYPE; }
  virtual parr_err_t val (u_int i, u_int *p) const { return PARR_BAD_TYPE; }
  virtual parr_err_t val (u_int i, char *p) const { return PARR_BAD_TYPE; }
  virtual parr_err_t val (u_int i, int16_t *p) const { return PARR_BAD_TYPE; }
  virtual parr_err_t val (u_int i, u_int16_t *p) const { return PARR_BAD_TYPE;}

  static ptr<parr_ival_t> alloc (const xpub_parr_t &x);
};

template<typename T, typename P> static parr_err_t 
_val (const vec<T> &v, u_int i, P *p)
{
  if (i < v.size ()) {
    *p = v[i];
    return PARR_OK;
  } else {
    return PARR_OUT_OF_BOUNDS;
  }
}

template<typename T>
class parr_ival_tmplt_t : public parr_ival_t {
public:
  parr_ival_tmplt_t (int64_t n, int64_t x) : minv (n), maxv (x) {}
  virtual ~parr_ival_tmplt_t () {} 

  parr_err_t val (u_int i, int64_t *p) const { return _val (v, i, p); }
  virtual parr_err_t val (u_int i, int *p) const { return PARR_OVERFLOW; }
  virtual parr_err_t val (u_int i, u_int *p) const { return PARR_OVERFLOW; }
  virtual parr_err_t val (u_int i, char *p) const { return PARR_OVERFLOW; }
  virtual parr_err_t val (u_int i, int16_t *p) const { return PARR_OVERFLOW; }
  virtual parr_err_t val (u_int i, u_int16_t *p) const { return PARR_OVERFLOW;}

  bool add (int64_t n) 
  { 
    if (n <= maxv && n >= minv) {
      v.push_back (n); 
      return true;
    } else {
      return false;
    }
  }

  u_int size () const { return v.size (); }

  bool to_xdr (xpub_val_t *x) const 
  {
    x->set_typ (XPUB_VAL_IARR);
    return to_xdr (x->iarr);
  }

  virtual bool to_xdr (xpub_parr_t *x) const = 0;

  template<typename R> 
  bool to_xdr (union_entry<rpc_vec<R, RPC_INFINITY> > x) const
  {
    u_int lim = v.size ();
    x->setsize (lim);
    for (u_int i = 0; i < lim; i++) (*x)[i] = static_cast<R> (v[i]);
    return true;
  }

  template<typename R> 
  void fill (const rpc_vec<R, RPC_INFINITY> &rpcv)
  {
    u_int lim = rpcv.size ();
    v.setsize (lim);
    for (u_int i = 0; i < lim; i++) v[i] = static_cast<T> (rpcv[i]);
  }

  str eval_simple () const
  {
    strbuf b;
    u_int lim = v.size ();
    b << "ARR=(";
    for (u_int i = 0; i < lim; i++) {
      if (i != 0) b << ", ";
      b << v[i];
    }
    b << ")";
    return b;
  }

  void eval_obj (pbuf_t *p, penv_t *e, u_int d) const
  {
    p->add (eval_simple ());
  }

  void dump2 (dumper_t *d) const
  {
    DUMP (d, "elements: " << eval_simple ());
  }

protected:
  int64_t minv, maxv;
  vec<T> v;
};

//
// class name: _int_
// ctype: int
// lower: INT_MIN
// upper: INT_MAX
// pointer casts: int
//
class parr_int_t : public parr_ival_tmplt_t<int> {
public:
  parr_int_t () : parr_ival_tmplt_t<int> (INT_MIN, INT_MAX) {}
  parr_int_t (const xpub_parr_int_t &x) : 
    parr_ival_tmplt_t<int> (INT_MIN, INT_MAX) { fill (x); }
  ptr<parr_int_t> to_int () { return mkref (this); }
  int operator[] (u_int i) const { return v[i]; }
  bool to_xdr (xpub_parr_t *x) const;
  str get_obj_name () const { return "parr_int_t"; }

  parr_err_t val (u_int i, int *p) const { return _val (v, i, p); }
};

class parr_char_t : public parr_ival_tmplt_t<char> {
public:
  parr_char_t () : parr_ival_tmplt_t<char> (CHAR_MIN, CHAR_MAX) {}
  parr_char_t (const xpub_parr_char_t &x) 
    : parr_ival_tmplt_t<char> (CHAR_MIN, CHAR_MAX) { fill (x); }
  ptr<parr_char_t> to_char () { return mkref (this); }
  char operator[] (u_int i ) const { return v[i]; }
  bool to_xdr (xpub_parr_t *x) const;
  str get_obj_name () const { return "parr_char_t"; }

  parr_err_t val (u_int i, int *p) const { return _val (v, i, p); }
  parr_err_t val (u_int i, char *p) const { return _val (v, i, p); }
  parr_err_t val (u_int i, int16_t *p) const { return _val (v, i, p); }
};

class parr_int16_t : public parr_ival_tmplt_t<int16_t> {
public:
  parr_int16_t () : parr_ival_tmplt_t<int16_t> (SHRT_MIN, SHRT_MAX) {}
  parr_int16_t (const xpub_parr_int_t &x) 
    : parr_ival_tmplt_t<int16_t> (SHRT_MIN, SHRT_MAX) { fill (x); }
  ptr<parr_int16_t> to_int16 () { return mkref (this); }
  int16_t operator[] (u_int i) const { return v[i]; }
  bool to_xdr (xpub_parr_t *x) const;
  str get_obj_name () const { return "parr_int16_t"; }

  parr_err_t val (u_int i, int16_t *p) const { return _val (v, i, p); }
  parr_err_t val (u_int i, int *p) const { return _val (v, i, p); }
};

class parr_uint_t : public parr_ival_tmplt_t<u_int> {
public:
  parr_uint_t () : parr_ival_tmplt_t<u_int> (0, UINT_MAX) {}
  parr_uint_t (const xpub_parr_uint_t &x) 
    : parr_ival_tmplt_t<u_int> (0, UINT_MAX) { fill (x); }
  ptr<parr_uint_t> to_uint () { return mkref (this); }
  u_int operator[] (u_int i) const { return v[i]; }
  bool to_xdr (xpub_parr_t *x) const;
  str get_obj_name () const { return "parr_uint_t"; }

  parr_err_t val (u_int i, u_int *p) const { return _val (v, i, p); }
};

class parr_uint16_t : public parr_ival_tmplt_t<u_int16_t> {
public:
  parr_uint16_t () : parr_ival_tmplt_t<u_int16_t> (0, USHRT_MAX) {}
  parr_uint16_t (const xpub_parr_int_t &x) 
    : parr_ival_tmplt_t<u_int16_t> (0, USHRT_MAX) { fill (x); }
  ptr<parr_uint16_t> to_uint16 () { return mkref (this); }
  u_int16_t operator[] (u_int i) const { return v[i]; }
  bool to_xdr (xpub_parr_t *x) const;
  str get_obj_name () const { return "parr_uint16_t"; }

  parr_err_t val (u_int i, u_int *p) const { return _val (v, i, p); }
  parr_err_t val (u_int i, u_int16_t *p) const { return _val (v, i, p); }
};

class parr_int64_t : public parr_ival_tmplt_t<int64_t> {
public:
  parr_int64_t () : parr_ival_tmplt_t<int64_t> (INT64_MIN, INT64_MAX) {}
  parr_int64_t (const xpub_parr_int64_t &x) 
    : parr_ival_tmplt_t<int64_t> (INT64_MIN, INT64_MAX) { fill (x); }
  ptr<parr_int64_t> to_int64 () { return mkref (this); }
  int64_t operator[] (u_int i) const { return v[i]; }
  bool to_xdr (xpub_parr_t *x) const;
  str get_obj_name () const { return "parr_int64_t"; }
};

class parr_mixed_t : public parr_t {
public:
  parr_mixed_t () : parr_t () {}
  parr_mixed_t (const xpub_parr_mixed_t &x);
  ~parr_mixed_t () {}

  const parr_mixed_t *to_mixed_arr () const { return this; }

  ptr<pval_t> operator[] (u_int i) const { return v[i]; }
  u_int size () const { return v.size (); }

  bool add (ptr<pval_t> i) { v.push_back (i); return true;}
  bool add (int64_t i) {v.push_back (New refcounted<pint_t> (i)); return true;}

  str get_obj_name () const { return "parr_t"; }
  void dump2 (dumper_t *d) const;
  bool to_xdr (xpub_val_t *x) const;

  str eval_simple () const;
  void eval_obj (pbuf_t *p, penv_t *e, u_int d) const;

private:
  vec<ptr<pval_t> > v;
};

#endif /* _LIBPUB_PARR_H */
