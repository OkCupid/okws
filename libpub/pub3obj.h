// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3expr.h"
#include "okformat.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  //
  // In internal class to implement an object refering to another
  // object.  Applications should never be using these objects
  // directly.
  //
  class oref_t {
  public:
    oref_t () {}
    virtual ~oref_t () {}
    virtual void set (ptr<expr_t> v) = 0;
    virtual ptr<expr_t> get () = 0;
    virtual ptr<const expr_t> get () const = 0;
  };

  //-----------------------------------------------------------------------

  class obj_ref_dict_t : public oref_t {
  public:
    obj_ref_dict_t (ptr<expr_dict_t> d, const str &k) : _dict (d), _key (k) {}
    void set (ptr<expr_t> v);
    ptr<expr_t> get () { return dict ()->lookup_ptr (_key); }
    ptr<const expr_t> get () const { return dict ()->lookup_ptr (_key); }
    
    static ptr<obj_ref_t> alloc (ptr<expr_dict_t> d, const str &k)
    { return New refcounted<obj_ref_dict_t> (d, k); }

    ptr<expr_dict_t> dict () { return _dict; }
    ptr<const expr_dict_t> dict () const { return _dict; }

  private:
    const ptr<expr_dict_t> _dict;
    const str _key;
  };

  //-----------------------------------------------------------------------

  class obj_ref_vec_t : public obj_ref_t {
  public:
    obj_ref_vec_t (ptr<vec_iface_t> v, size_t i) : _vec (v), _index (i) {}
    void set (ptr<expr_t> v);
    ptr<expr_t> get ();
    ptr<const expr_t> get () const;

    static ptr<obj_ref_t> alloc (ptr<vec_iface_t> v, size_t i)
    { return New refcounted<obj_ref_vec_t> (v, i); }

  private:
    const ptr<vec_iface_t> _vec;
    const size_t _index;
  };

  //-----------------------------------------------------------------------

#define I2S(typ, prnt) \
  inline str i2s (typ i) { return strbuf ("%" prnt, i); }

  I2S(int64_t, PRId64)
  I2S(int32_t, PRId32)
  I2S(int16_t, PRId16)
  I2S(int8_t, PRId8)
  I2S(u_int64_t, PRIu64)
  I2S(u_int32_t, PRIu32)
  I2S(u_int16_t, PRIu16)
  I2S(u_int8_t, PRIu8)

#undef I2S

  //-----------------------------------------------------------------------

  class const_obj_t {
  public:
    const_obj_t (ptr<expr_t> x) : _c_obj (x) {}
    const_obj_t (ptr<const expr_t> x) : _c_obj (x) {}
    const_obj_t (ptr<obj_ref_t> r);
    const_obj_t (ptr<const obj_ref_t>  r);
    const_obj_t (ptr<expr_t> v) : _c_obj (v) {}
    const_obj_t (ptr<const expr_t> v) : _c_obj (v) {}
    const_obj_t () {}
    virtual ~const_obj_t () {}

    size_t size () const;
    const_obj_t operator[] (size_t s) const;
    const_obj_t operator() (const str &s) const;

    ALL_INT_TYPES (const_obj_t operator(), i, 
		   const { return (*this)(i2s (i)); });

    virtual ptr<const expr_t> obj () const { return _c_obj; }
    bool isnull () const { return !_c_obj; }

    bool to_int (int64_t *i) const;
    bool to_uint (u_int64_t *u) const;
    bool to_str (str *s) const;
    bool to_bool (bool *b) const;

    str to_str () const;
    bool to_bool () const;
    int64_t to_int () const;
    u_int64_t to_uint () const;

  protected:
    ptr<const expr_list_t> to_vector () const;
    ptr<const expr_dict_t> to_dict () const;

  protected:
    ptr<const expr_t> _c_obj;
  };

  //-----------------------------------------------------------------------

  class obj_t : public const_obj_t {
  public:
    obj_t (ptr<obj_ref_t> r) : const_obj_t (r), _ref (r) {}
    obj_t (ptr<expr_t> v) : const_obj_t (v), _obj (v) {}
    obj_t (ptr<expr_t> e) : const_obj_t (e), _obj (e) {}
    obj_t () {}

    // array access features: mutable
    obj_t push_back ();
    void push_back (obj_t o);
    obj_t operator[] (size_t s);

    // dict access features: mutable
    void insert (const str &n, obj_t o);
    obj_t operator() (const str &s);
    const_obj_t operator() (const str &s) const;
    const_obj_t operator[] (size_t s) const;

    ALL_INT_TYPES (const_obj_t operator(), i, 
		   const { return (*this)(i2s (i)); });
    ALL_INT_TYPES (obj_t operator(), i, { return (*this)(i2s (i)); });

    // scalar access features: mutable
    obj_t &operator= (u_int64_t i) { return set_uint (i); }
    obj_t &operator= (const str &s) { return set_str (s); }
    obj_t &operator= (double d) { return set_double (d); }
    obj_t &operator= (ptr<expr_t> z) { return set_value (z); }
    obj_t &operator= (scalar_obj_t o) { return set_scalar (o); }

    // Use implicit conversions, converting strings that look
    // like ints into ints. Look for problems with strings of
    // the form '0533' which will be looked on as octal.
    obj_t &fancy_assign_to (const str &s);

    ALL_INT_TYPES(obj_t &operator=, i, { return set_int (i); })

    // wild card
    obj_t &operator= (obj_t o) { return set_obj (o); }

    obj_t &refer_to (obj_t o);

    ptr<aarr_t> dict ();
    ptr<const aarr_t> dict () const;

    ptr<const expr_t> obj () const;
    ptr<const expr_t> expr () const;
    ptr<expr_t> obj ();
    void clear ();

    bool to_int (int64_t *i) const;
    bool to_uint (u_int64_t *u) const;
    bool to_str (str *s, bool eval = true) const;
    bool to_bool (bool *b) const;

    str to_str (bool eval = true) const;
    bool to_bool () const;
    int64_t to_int () const;
    u_int64_t to_uint () const;

    bool append (obj_t in);
    
  protected:

    // Mutations
    ptr<expr_list_t> to_vector () ;
    ptr<expr_dict_t> to_dict ();

    obj_t &set_value (ptr<expr_t> v);
    obj_t &set_scalar_expr (ptr<expr_t> e);

    obj_t &set_int (int64_t i);
    obj_t &set_uint (u_int64_t i);
    obj_t &set_str (const str &s);
    obj_t &set_double (double d);
    obj_t &set_obj (obj_t o);
    obj_t &set_scalar (scalar_obj_t so);

    void update_value (ptr<expr_t> v);
    ptr<expr_t> value () { return obj (); }

    bool set_value_vec (ptr<expr_t> v);
    bool set_value_scalar (ptr<expr_t> v);
    bool set_value_dict (ptr<expr_t> v);
    void clear_value ();

    ptr<expr_t> _obj;
    ptr<obj_ref_t> _ref;
    ptr<expr_list_t> _vec;
    ptr<expr_dict_t> _dict;
    ptr<expr_t> _scalar;
  };
  
  //-----------------------------------------------------------------------

  //
  // Applications should use this class when handling references to pub3
  // objects. Consider:
  //
  //   pub3::obj_t o;
  //   pub3::ref_t r = o.push_back ();
  //
  class obj_ref_t : public obj_t {
  public:
    obj_ref_t (ptr<ref_t> r) : obj_t (r) {}
    obj_ref_t (const obj_t &o) { refer_to (o); }
  private:
    // Should never be used!
    obj_t &operator= (obj_t o);
  };

  //-----------------------------------------------------------------------

  class obj_list_t : public obj_t {
  public:
    obj_list_t ();
  };


  //-----------------------------------------------------------------------

  class obj_dict_t : public obj_t {
  public:
    obj_dict_t ();
  };

  //-----------------------------------------------------------------------


  //
  // A system embedded deep within OkCupid code uses a customized
  // form of dictionary, that inherits from aarr_arg_t.  This template
  // allows us to accommodate such a subclass.
  //
  template<class D>
  class obj_dict_tmplt_t : public obj_t {
  public:
    obj_dict_tmplt_t ()
    {
      _dict_tmplt = New refcounted<expr_dict_tmplt_t<D> > ();
      _c_obj = _obj = _dict = _dict_tmplt;
    }
    ptr<D> dict_tmplt () { return _dict_tmplt->dict_tmplt (); }
    ptr<const D> dict_tmplt () const { return _dict_tmplt->dict_tmplt (); }
  protected:
    ptr<expr_dict_tmplt_t<D> > _dict_tmplt;
  };


  //-----------------------------------------------------------------------

};


#endif /* _LIBPUB_PUB3OBJ_H_ */

