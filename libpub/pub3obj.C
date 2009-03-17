
#include "pub3obj.h"

namespace pub3 {

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
      return const_obj_t (d->value_ref (s));
    }
    return const_obj_t ();
  }

  //-----------------------------------------------------------------------

  ptr<const parr_mixed_t>
  const_obj_t::to_vector () const
  {
    ptr<const parr_mixed_t> v;
    const parr_mixed_t *vr;
    if ((vr = _const_val_ref->to_mixed_arr ())) {
      v = mkref (vr);
    }
    return v;
  }

  //-----------------------------------------------------------------------

  ptr<const aarr_arg_t>
  const_obj_t::to_dict () const
  {
    return _const_val_ref->to_aarr ();
  }

  //-----------------------------------------------------------------------

  ptr<const pub_scalar_t>
  const_obj_t::to_scalar () const
  {
    return _const_val_ref->to_scalar ();
  }

  //=======================================================================


  obj_t
  obj_t::push_back ()
  {
    ptr<parr_mixed_t> v = to_vector ();
    obj_t o (v->push_back ());
    return o;
  }

  //-----------------------------------------------------------------------

  void
  obj_t::push_back (obj_t o)
  {
    ptr<parr_mixed_t> v = to_vector ();
    v->add (o.value ());
  }

  //-----------------------------------------------------------------------

  obj_t
  obj_t::operator[] (size_t i)
  {
    ptr<parr_mixed_t> v = to_vector ();
    if (v->size () <= i) {
      v->setsize (i + 1);
    }
    return obj_t ((*v)[i]);
  }

  //-----------------------------------------------------------------------

  void
  obj_t::insert (const str &n, obj_t o)
  {
    ptr<aarr_arg_t> d = to_dict ();
    d->replace (n, o.value ());
  }

  //-----------------------------------------------------------------------

  obj_t
  obj_t::operator() (const str &s)
  {
    ptr<aarr_arg_t> d = to_dict ();
    return obj_t (d->value_ref (s));
  }

  //-----------------------------------------------------------------------

  ptr<parr_mixed_t> 
  obj_t::to_vector ()
  {
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
    if (!_scalar) {
      _vec = NULL;
      _dict = NULL;
      _scalar = New refcounted<pub_scalar_t> ();
      update_value (_scalar);
    }
    return _scalar;
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_value (ptr<pval_t> in)
  {
    set_value_scalar (in) || set_value_dict (in) || set_value_vec (in);
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
    parr_mixed_t *vr = in->to_mixed_arr ();
    if (vr) {
      ptr<parr_mixed_t> v = mkref (vr);
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
};
