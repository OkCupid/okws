// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#ifndef _LIBPUB_PUB3OBJ_H_
#define _LIBPUB_PUB3OBJ_H_

#include "pub.h"
#include "parr.h"
#include "pub3expr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class obj_ref_t {
  public:
    obj_ref_t () {}
    virtual ~obj_ref_t () {}
    virtual void set (ptr<pval_t> v) = 0;
    virtual ptr<pval_t> get () = 0;
    virtual ptr<const pval_t> get () const = 0;
  };

  //-----------------------------------------------------------------------

  class obj_ref_dict_t : public obj_ref_t {
  public:
    obj_ref_dict_t (ptr<expr_dict_t> d, const str &k) : _dict (d), _key (k) {}
    void set (ptr<pval_t> v) { dict ()->replace (_key, v); }
    ptr<pval_t> get () { return dict ()->lookup_ptr (_key); }
    ptr<const pval_t> get () const { return dict ()->lookup_ptr (_key); }
    
    static ptr<obj_ref_t> alloc (ptr<expr_dict_t> d, const str &k)
    { return New refcounted<obj_ref_dict_t> (d, k); }

    ptr<aarr_t> dict () { return _dict->dict (); }
    ptr<const aarr_t> dict () const { return _dict->dict (); }

  private:
    const ptr<expr_dict_t> _dict;
    const str _key;
  };

  //-----------------------------------------------------------------------

  class obj_ref_vec_t : public obj_ref_t {
  public:
    obj_ref_vec_t (ptr<vec_iface_t> v, size_t i) : _vec (v), _index (i) {}
    void set (ptr<pval_t> v);
    ptr<pval_t> get ();
    ptr<const pval_t> get () const;

    static ptr<obj_ref_t> alloc (ptr<vec_iface_t> v, size_t i)
    { return New refcounted<obj_ref_vec_t> (v, i); }

  private:
    const ptr<vec_iface_t> _vec;
    const size_t _index;
  };

  //-----------------------------------------------------------------------

  class const_obj_t {
  public:
    const_obj_t () {}
    const_obj_t (ptr<const pval_t> v) : _c_obj (v) {}
    virtual ~const_obj_t () {}

    size_t size () const;
    const_obj_t operator[] (size_t s) const;
    const_obj_t operator() (const str &s) const;

    virtual ptr<const pval_t> obj () const { return _c_obj; }

  protected:
    ptr<const expr_list_t> to_vector () const;
    ptr<const expr_dict_t> to_dict () const;

  private:
    ptr<const pval_t> _c_obj;
  };

  //-----------------------------------------------------------------------

  class obj_t : public const_obj_t {
  public:
    obj_t (ptr<obj_ref_t> r) : _ref (r) {}
    obj_t (ptr<pval_t> v) : _obj (v) {}
    obj_t () {}

    // array access features: mutable
    obj_t push_back ();
    void push_back (obj_t o);
    obj_t operator[] (size_t s);

    // dict access features: mutable
    void insert (const str &n, obj_t o);
    obj_t operator() (const str &s);

    // scalar access features: mutable
    obj_t &operator= (u_int64_t i) { return set_uint (i); }
    obj_t &operator= (const str &s) { return set_str (s); }
    obj_t &operator= (double d) { return set_double (d); }
    obj_t &operator= (ptr<pval_t> z) { return set_value (z); }
    obj_t &operator= (scalar_obj_t o) { return set_scalar (o); }

    ALL_INT_TYPES(obj_t &operator=, i, { return set_int (i); })

    // wild card
    obj_t &operator= (obj_t o) { return set_obj (o); }

    ptr<aarr_t> dict ();
    ptr<const aarr_t> dict () const;

    ptr<const pval_t> obj () const;
    ptr<pval_t> obj ();
    
  protected:

    // Mutations
    ptr<expr_list_t> to_vector () ;
    ptr<expr_dict_t> to_dict ();

    obj_t &set_value (ptr<pval_t> v);
    obj_t &set_scalar_expr (ptr<expr_t> e);

    obj_t &set_int (int64_t i);
    obj_t &set_uint (u_int64_t i);
    obj_t &set_str (const str &s);
    obj_t &set_double (double d);
    obj_t &set_obj (obj_t o);
    obj_t &set_scalar (scalar_obj_t so);

    void update_value (ptr<pval_t> v);
    ptr<pval_t> value () { return obj (); }

    bool set_value_vec (ptr<pval_t> v);
    bool set_value_scalar (ptr<pval_t> v);
    bool set_value_dict (ptr<pval_t> v);
    void clear_value ();

  private:
    ptr<pval_t> _obj;
    ptr<obj_ref_t> _ref;
    ptr<expr_list_t> _vec;
    ptr<expr_dict_t> _dict;
    ptr<expr_t> _scalar;
  };
  
  //-----------------------------------------------------------------------

};


#endif /* _LIBPUB_PUB3OBJ_H_ */

