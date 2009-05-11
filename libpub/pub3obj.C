#include "pub3obj.h"

namespace pub3 {

  //=======================================================================

  void
  obj_ref_dict_t::set (ptr<pval_t> v)
  {
    if (!v) v = expr_null_t::alloc ();
    dict ()->replace (_key, v);
  }

  //-----------------------------------------------------------------------

  void
  obj_ref_vec_t::set (ptr<pval_t> v)
  {
    if (_vec) {
      if (_vec->size () <= _index) {
	_vec->setsize (_index + 1);
      }
      _vec->set (_index, v);
    }
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t>
  obj_ref_vec_t::get () const
  {
    ptr<const pval_t> v;
    if (_vec && _index < _vec->size ()) { v = _vec->lookup (_index); }
    return v;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  obj_ref_vec_t::get () 
  {
    ptr<pval_t> v;
    if (_vec && _index < _vec->size ()) { v = _vec->lookup (_index); }
    return v;
  }

  //=======================================================================

  size_t
  const_obj_t::size () const 
  {
    size_t r = 0;
    ptr<const expr_list_t> l;
    ptr<const expr_dict_t> d;

    if ((l = to_vector ())) { r = l->size (); }
    else if ((d = to_dict ())) { r = d->size (); }

    return r;
  }

  //-----------------------------------------------------------------------

  const_obj_t
  const_obj_t::operator[] (size_t i) const
  {
    ptr<const expr_list_t> v = to_vector ();
    if (v && i < v->size ()) {
      return const_obj_t ((*v)[i]);
    }
    return const_obj_t ();
  }

  //-----------------------------------------------------------------------

  const_obj_t
  const_obj_t::operator() (const str &s) const
  {
    ptr<const expr_dict_t> d = to_dict ();
    if (d) {
      return const_obj_t (d->dict ()->lookup_ptr (s));
    }
    return const_obj_t ();
  }

  //-----------------------------------------------------------------------

  ptr<const expr_list_t>
  const_obj_t::to_vector () const
  {
    ptr<const expr_list_t> r;
    if (obj ()) r = obj ()->to_expr_list ();
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_dict_t>
  const_obj_t::to_dict () const
  {
    ptr<const expr_dict_t> r;
    if (obj ()) r = obj ()->to_expr_dict ();
    return r;
  }

  //=======================================================================

  obj_t
  obj_t::push_back ()
  {
    ptr<expr_list_t> v = to_vector ();
    v->push_back ();
    obj_t o (obj_ref_vec_t::alloc (v, v->size () - 1));
    return o;
  }

  //-----------------------------------------------------------------------

  void
  obj_t::push_back (obj_t o)
  {
    ptr<expr_list_t> v = to_vector ();
    v->push_back (o.obj ());
  }

  //-----------------------------------------------------------------------

  obj_t
  obj_t::operator[] (size_t i)
  {
    ptr<expr_list_t> v = to_vector ();
    return obj_t (obj_ref_vec_t::alloc (v, i)); 
  }

  //-----------------------------------------------------------------------

  void
  obj_t::insert (const str &n, obj_t o)
  {
    ptr<expr_dict_t> d = to_dict ();
    d->dict ()->replace (n, o.obj ());
  }

  //-----------------------------------------------------------------------

  obj_t
  obj_t::operator() (const str &s)
  {
    ptr<expr_dict_t> d = to_dict ();
    return obj_t (obj_ref_dict_t::alloc (d, s));
  }

  //-----------------------------------------------------------------------

  ptr<expr_list_t> 
  obj_t::to_vector ()
  {
    if (!_vec && obj ()) {
      _vec = obj ()->to_expr_list ();
    }

    if (!_vec) {
      _scalar = NULL;
      _dict = NULL;
      _vec = New refcounted<expr_list_t> ();
      update_value (_vec);
    }
    return _vec;
  }

  //-----------------------------------------------------------------------

  obj_dict_t::obj_dict_t ()
  {
    _dict = New refcounted<expr_dict_t> ();
    _c_obj = _obj = _dict;
  }

  //-----------------------------------------------------------------------

  obj_list_t::obj_list_t ()
  {
    _vec = New refcounted<expr_list_t> ();
    _c_obj = _obj = _vec;
  }


  //-----------------------------------------------------------------------

  ptr<expr_dict_t>
  obj_t::to_dict ()
  {
    if (!_dict && obj ()) {
      _dict = obj ()->to_expr_dict ();
    }

    if (!_dict) {
      _scalar = NULL;
      _vec = NULL;
      _dict = New refcounted<expr_dict_t> ();
      update_value (_dict);
    }
    return _dict;
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_value (ptr<pval_t> in)
  {
    if (!set_value_dict (in) && !set_value_vec (in)) {
      set_value_scalar (in);
    }
    return (*this);
  }

  //----------------------------------------------------------------------

  void
  obj_t::clear_value ()
  {
    _vec = NULL;
    _dict = NULL;
    _scalar = NULL;
    _c_obj = _obj = NULL;
  }

  //----------------------------------------------------------------------

  void
  obj_t::clear () 
  {
    _ref = NULL;
    _c_obj = NULL;
    _obj = NULL;
    _vec = NULL;
    _dict = NULL;
    _scalar = NULL;
  }

  //----------------------------------------------------------------------

  obj_t &
  obj_t::refer_to (obj_t in)
  {
    _ref = in._ref;
    _c_obj = _obj = in._obj;
    _vec = in._vec;
    _dict = in._dict;
    _scalar = in._scalar;
    return (*this);
  }

  //----------------------------------------------------------------------

  obj_t &
  obj_t::set_obj (obj_t in)
  {

    // Tricky: what happens when A is a reference, B is a reference
    // and we assign A = B? 
    //
    // We're going to mimic C++-style semantics.  If A is just being
    // constructed, then assign A's reference to point to B's reference.
    // In such a case, _ref will be NULL and in._ref
    // will be true.
    //
    // If A is being reassigned, then set *A equal to whatever B
    // references.
    //
    
    if (_ref && !in._ref) {
      _ref->set (in.obj ()); 
    } else {
      _ref = in._ref;
      _c_obj = _obj = in._obj;
      _vec = in._vec;
      _dict = in._dict;
      _scalar = in._scalar;
    }
    return (*this);
  }

  //----------------------------------------------------------------------

  bool
  obj_t::set_value_scalar (ptr<pval_t> in)
  {
    bool ret = true;
    if (in) {
      _vec = NULL;
      _dict = NULL;
      _scalar = in->to_expr ();
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
    ptr<expr_list_t> v = in->to_expr_list ();
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
    ptr<expr_dict_t> d = in->to_expr_dict ();
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
  obj_t::set_scalar_expr (ptr<expr_t> e)
  {
    _vec = NULL;
    _dict = NULL;
    _scalar = e;
    update_value (_scalar);
    return (*this);
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_scalar (scalar_obj_t so)
  {
    ptr<expr_t> p = expr_t::alloc (so);
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
    return set_scalar_expr (expr_int_t::alloc (i));
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_uint (u_int64_t i)
  {
    return set_scalar_expr (expr_uint_t::alloc (i));
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::fancy_assign_to (const str &s)
  {
    return set_scalar_expr (expr_t::alloc (s));
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_str (const str &s)
  {
    return set_scalar_expr (expr_str_t::alloc (s));
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_double (double d)
  {
    return set_scalar_expr (expr_double_t::alloc (d));
  }

  //-----------------------------------------------------------------------

  void
  obj_t::update_value (ptr<pval_t> v)
  {
    if (_ref) { _ref->set (v); }
    else      { _c_obj = _obj = v; }
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

  ptr<aarr_t>
  obj_t::dict () 
  {
    ptr<aarr_t> d;
    if (_dict) d = _dict->dict ();
    return d;
  }

  //-----------------------------------------------------------------------

  ptr<const aarr_t>
  obj_t::dict () const
  {
    ptr<const aarr_t> d;
    if (_dict) d = _dict->dict ();
    return d;
  }

  //-----------------------------------------------------------------------

  const_obj_t::const_obj_t (ptr<const obj_ref_t> r)
    : _c_obj (r ? r->get () : ptr<const pval_t> ())
  {}

  //-----------------------------------------------------------------------

  const_obj_t::const_obj_t (ptr<obj_ref_t> r)
    : _c_obj (r ? r->get () : ptr<pval_t> ())
  {}

  //-----------------------------------------------------------------------

  const_obj_t
  obj_t::operator() (const str &s) const
  {
    const_obj_t co = (*this);
    return co(s);
  }

  //-----------------------------------------------------------------------

  const_obj_t
  obj_t::operator[] (size_t s) const
  {
    const_obj_t co = (*this);
    return co[s];
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::to_int (int64_t *i) const
  {
    return (obj () && obj ()->to_int64 (i));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  obj_t::expr () const 
  {
    ptr<const expr_t> x;
    ptr<const pval_t> o = obj ();
    if (o) x = o->to_expr ();
    return x;
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::to_uint (u_int64_t *u) const
  {
    scalar_obj_t so;
    bool ret = false;
    ptr<const expr_t> x;

    if (_scalar) {
      so = _scalar->to_scalar ();
      ret = so.to_uint64 (u);
    } else if ((x = expr ())) {
      ret = x->to_uint (u);
    }

    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::to_str (str *s) const
  {
    bool ret;
    ptr<const expr_t> x;

    if (_scalar) {
      ret = (_scalar && (*s = _scalar->to_str ()));
    } else if ((x = expr ())) {
      ret = (*s = x->to_str ());
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::to_bool (bool *b) const
  {
    bool ret = false;
    ptr<const expr_t> x;
    if (_scalar) {
      ret = _scalar && (*b = _scalar->to_bool ());
    } else if ((x = expr ())) {
      ret = (x && (*b = x->to_bool ()));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  str
  obj_t::to_str () const 
  {
    str ret;
    to_str (&ret);
    return ret;
  }

  //-----------------------------------------------------------------------
  
  bool
  obj_t::to_bool () const
  {
    bool ret = false;
    to_bool (&ret);
    return ret;
  }

  //-----------------------------------------------------------------------

  int64_t
  obj_t::to_int () const
  {
    int64_t r = 0;
    to_int (&r);
    return r;
  }

  //-----------------------------------------------------------------------

  u_int64_t
  obj_t::to_uint () const
  {
    u_int64_t r = 0;
    to_uint (&r);
    return r;
  }

  //-----------------------------------------------------------------------

  bool
  const_obj_t::to_int (int64_t *i) const
  {
    return _c_obj && _c_obj->to_int64 (i);
  }

  //-----------------------------------------------------------------------

  bool
  const_obj_t::to_uint (u_int64_t *i) const
  {
    ptr<const expr_t> x;
    return _c_obj && (x = _c_obj->to_expr ()) && x->to_uint (i);
  }

  //-----------------------------------------------------------------------

  bool
  const_obj_t::to_bool (bool *b) const
  {
    return _c_obj && (*b = _c_obj->to_bool ());
  }

  //-----------------------------------------------------------------------

  bool
  const_obj_t::to_str (str *s) const
  {
    ptr<const expr_t> x;
    return _c_obj && (x = _c_obj->to_expr ()) && (*s = x->to_str ());
  }

  //-----------------------------------------------------------------------

  str
  const_obj_t::to_str () const
  {
    str s;
    to_str (&s);
    return s;
  }

  //-----------------------------------------------------------------------

  bool
  const_obj_t::to_bool () const
  {
    bool b;
    to_bool (&b);
    return b;
  }

  //-----------------------------------------------------------------------

  int64_t
  const_obj_t::to_int () const
  {
    int64_t i = 0;
    to_int (&i);
    return i;
  }

  //-----------------------------------------------------------------------

  u_int64_t 
  const_obj_t::to_uint () const
  {
    u_int64_t u = 0;
    to_uint (&u);
    return u;
  }

  //-----------------------------------------------------------------------

};
