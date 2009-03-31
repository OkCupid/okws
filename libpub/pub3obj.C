#include "pub3obj.h"

namespace pub3 {

  //=======================================================================

  void
  obj_ref_vec_t::set (ptr<pval_t> v)
  {
    if (_vec) {
      if (_vec->size () <= _index) {
	_vec->setsize (_index + 1);
      }
      (*_vec)[_index] = v;
    }
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t>
  obj_ref_vec_t::get () const
  {
    ptr<const pval_t> v;
    if (_vec && _index < _vec->size ()) { v = (*_vec)[_index]; }
    return v;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  obj_ref_vec_t::get () 
  {
    ptr<pval_t> v;
    if (_vec && _index < _vec->size ()) { v = (*_vec)[_index]; }
    return v;
  }

  //=======================================================================

  size_t
  const_obj_t::size () const 
  {
    size_t r = 0;
    ptr<const parr_mixed_t> v;
    ptr<const aarr_arg_t> d;

    if ((v = to_vector ())) { r = v->size (); }
    else if ((d = to_dict ())) { r = d->size (); }

    return r;
  }

  //-----------------------------------------------------------------------

  const_obj_t
  const_obj_t::operator[] (size_t i) const
  {
    ptr<const parr_mixed_t> v = to_vector ();
    if (v && i < v->size ()) {
      return const_obj_t ((*v)[i]);
    }
    return const_obj_t ();
  }

  //-----------------------------------------------------------------------

  const_obj_t
  const_obj_t::operator() (const str &s) const
  {
    ptr<const aarr_arg_t> d = to_dict ();
    if (d) {
      return const_obj_t (d->lookup_ptr (s));
    }
    return const_obj_t ();
  }

  //-----------------------------------------------------------------------

  ptr<const parr_mixed_t>
  const_obj_t::to_vector () const
  {
    ptr<const parr_mixed_t> r;
    if (obj ()) r = obj ()->to_mixed_arr ();
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<const aarr_arg_t>
  const_obj_t::to_dict () const
  {
    ptr<const aarr_arg_t> r;
    if (obj ()) r = obj ()->to_aarr ();
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<const pub_scalar_t>
  const_obj_t::to_scalar () const
  {
    ptr<const pub_scalar_t> r;
    if (obj ()) r = obj ()->to_scalar ();
    return r;
  }

  //=======================================================================

  obj_t
  obj_t::push_back ()
  {
    ptr<parr_mixed_t> v = to_vector ();
    v->push_back ();
    obj_t o (obj_ref_vec_t::alloc (v, v->size () - 1));
    return o;
  }

  //-----------------------------------------------------------------------

  void
  obj_t::push_back (obj_t o)
  {
    ptr<parr_mixed_t> v = to_vector ();
    v->add (o.obj ());
  }

  //-----------------------------------------------------------------------

  obj_t
  obj_t::operator[] (size_t i)
  {
    ptr<parr_mixed_t> v = to_vector ();
    return obj_t (obj_ref_vec_t::alloc (v, i)); 
  }

  //-----------------------------------------------------------------------

  void
  obj_t::insert (const str &n, obj_t o)
  {
    ptr<aarr_arg_t> d = to_dict ();
    d->replace (n, o.obj ());
  }

  //-----------------------------------------------------------------------

  obj_t
  obj_t::operator() (const str &s)
  {
    ptr<aarr_arg_t> d = to_dict ();
    return obj_t (obj_ref_dict_t::alloc (d, s));
  }

  //-----------------------------------------------------------------------

  ptr<parr_mixed_t> 
  obj_t::to_vector ()
  {
    if (!_vec && obj ()) {
      _vec = obj ()->to_mixed_arr ();
    }

    if (!_vec) {
      _scalar = NULL;
      _dict = NULL;
      _vec = New refcounted<parr_mixed_t> ();
      update_value (_vec);
    }
    return _vec;
  }

  //-----------------------------------------------------------------------

  ptr<aarr_arg_t>
  obj_t::to_dict ()
  {
    if (!_dict && obj ()) {
      _dict = obj ()->to_aarr ();
    }

    if (!_dict) {
      _scalar = NULL;
      _vec = NULL;
      _dict = New refcounted<aarr_arg_t> ();
      update_value (_dict);
    }
    return _dict;
  }

  //-----------------------------------------------------------------------

  ptr<pub_scalar_t>
  obj_t::to_scalar ()
  {
    if (!_scalar && obj ()) {
      _scalar = obj ()->to_scalar ();
    }

    if (!_scalar) {
      _vec = NULL;
      _dict = NULL;
      _scalar = New refcounted<pub_scalar_t> ();
      update_value (_scalar);
    }
    return _scalar;
  }

  //-----------------------------------------------------------------------

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_value (ptr<pval_t> in)
  {
    if (!in) {
      /* noop */
    } else if (set_value_scalar (in)) {
      /* noop */ 
    } else if (set_value_dict (in)) {
      /* noop */ 
    } else {
      set_value_vec (in);
    }
    return (*this);
  }

  //----------------------------------------------------------------------

  void
  obj_t::clear_value ()
  {
    _vec = NULL;
    _dict = NULL;
    _obj = NULL;
  }

  //----------------------------------------------------------------------

  obj_t &
  obj_t::set_obj (obj_t in)
  {
    _ref = in._ref;
    _obj = in._obj;
    _vec = in._vec;
    _dict = in._dict;
    _scalar = in._scalar;
    return (*this);
  }

  //----------------------------------------------------------------------

  bool
  obj_t::set_value_scalar (ptr<pval_t> in)
  {
    bool ret = true;
    ptr<pub_scalar_t> s = in->to_scalar ();
    if (s) {
      _vec = NULL;
      _dict = NULL;
      _scalar = s;
      update_value (_scalar);
    } else {
      ret = false;
    } 
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::set_value_vec (ptr<pval_t> in)
  {
    bool ret = true;
    ptr<parr_mixed_t> v = in->to_mixed_arr ();
    if (v) {
      _dict = NULL;
      _scalar = NULL;
      _vec = v;
      update_value (_vec);
    } else {
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::set_value_dict (ptr<pval_t> in)
  {
    ptr<aarr_arg_t> d = in->to_aarr ();
    bool ret = true;
    if (d) {
      _vec = NULL;
      _scalar = NULL;
      _dict = d;
      update_value (d);
    } else {
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_scalar (scalar_obj_t so)
  {
    ptr<pub_scalar_t> p = New refcounted<pub_scalar_t> (so);
    _vec = NULL;
    _dict = NULL;
    _scalar = p;
    update_value (p);
    return (*this);
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_int (int64_t i)
  {
    scalar_obj_t so;
    so.set (i);
    return set_scalar (so);
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_uint (u_int64_t i)
  {
    scalar_obj_t so;
    so.set_u (i);
    return set_scalar (so);
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_str (const str &s)
  {
    scalar_obj_t so;
    so.set (s);
    return set_scalar (so);
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_double (double d)
  {
    scalar_obj_t so;
    so.set (d);
    return set_scalar (so);
  }

  //-----------------------------------------------------------------------

  void
  obj_t::update_value (ptr<pval_t> v)
  {
    if (_ref) { _ref->set (v); }
    else      { _obj = v; }
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t>
  obj_t::obj () const
  {
    if (_ref) { return _ref->get (); }
    else      { return _obj; }
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  obj_t::obj ()
  {
    if (_ref) { return _ref->get (); }
    else      { return _obj; }
  }

  //-----------------------------------------------------------------------

};
