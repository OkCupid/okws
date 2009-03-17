// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */


#ifndef _LIBPUB_PUBROW_H_
#define _LIBPUB_PUBROW_H_

#include "pub.h"
#include "parr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class const_obj_t {
  public:
    const_obj_t (const ptr<pval_t> &r) : _const_val_ref (r) {}
    const_obj_t () : _const_val_ref (_dummy_val) {}

    size_t size () const;
    const_obj_t operator[] (size_t s) const;
    const_obj_t operator() (const str &s) const;

  protected:
    ptr<const parr_mixed_t> to_vector () const;
    ptr<const aarr_arg_t> to_dict () const;
    ptr<const pub_scalar_t> to_scalar () const;

  private:
    ptr<pval_t> _dummy_val;
    const ptr<pval_t> &_const_val_ref;
  };

  //-----------------------------------------------------------------------

  class obj_t : public const_obj_t {
  public:
    obj_t (ptr<pval_t> &r) : const_obj_t (r), _val (r), _val_ref (r) {}
    obj_t () : const_obj_t (),  _val_ref (_val) {}

    // array access features: mutable
    obj_t push_back ();
    void push_back (obj_t o);
    obj_t operator[] (size_t s);

    // dict access features: mutable
    void insert (const str &n, obj_t o);
    obj_t operator() (const str &s);

    // scalar access features: mutable
    obj_t &operator= (int64_t i) { return set_int (i); }
    obj_t &operator= (int i) { return set_int (i); }
    obj_t &operator= (size_t s) { return set_int (s); }
    obj_t &operator= (const str &s) { return set_str (s); }
    obj_t &operator= (double d) { return set_double (d); }
    obj_t &operator= (ptr<pval_t> z) { return set_value (z); }
    obj_t &operator= (scalar_obj_t o) { return set_scalar (o); }

    // wild card
    obj_t &operator= (obj_t o) { return set_obj (o); }

    ptr<aarr_t> dict () { return _dict; }
    ptr<const aarr_t> dict () const { return _dict; }
    
  protected:

    // Mutations
    ptr<parr_mixed_t> to_vector () ;
    ptr<aarr_arg_t> to_dict ();
    ptr<pub_scalar_t> to_scalar ();

    obj_t &set_value (ptr<pval_t> v);

    obj_t &set_int (int64_t i);
    obj_t &set_str (const str &s);
    obj_t &set_double (double d);
    obj_t &set_obj (obj_t o) { return set_value (o.value ()); }
    obj_t &set_scalar (scalar_obj_t so);

    void update_value (ptr<pval_t> v) { _val_ref = v; _val = v; }
    ptr<pval_t> value () { return _val_ref; }

    bool set_value_vec (ptr<pval_t> v);
    bool set_value_scalar (ptr<pval_t> v);
    bool set_value_dict (ptr<pval_t> v);

  private:
    ptr<pval_t> _val;
    ptr<pval_t> &_val_ref;
    ptr<parr_mixed_t> _vec;
    ptr<aarr_arg_t> _dict;
    ptr<pub_scalar_t> _scalar;
  };
  
  //-----------------------------------------------------------------------

};


#endif /* _LIBPUB_PUBROW_H_ */

