#include "pub3obj.h"

namespace pub3 {

  //=======================================================================

  void
  obj_ref_dict_t::set (ptr<expr_t> x)
  {
    if (!x) x = expr_null_t::alloc ();
    dict ()->replace (_key, x);
  }

  //-----------------------------------------------------------------------

  void
  obj_ref_list_t::set (ptr<expr_t> x)
  {
    _list->set (_index, expr_t::safe_expr (x));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  obj_ref_list_t::get () const
  {
    ptr<const expr_t> x;
    if (_list && _index < _list->size ()) { x = _list->lookup (_index); }
    return x;
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  obj_ref_list_t::get () 
  {
    ptr<expr_t> x;
    if (_list && _index < _list ->size ()) { x = _list->lookup (_index); }
    return x;
  }

  //=======================================================================

  size_t
  const_obj_t::size () const 
  {
    size_t r = 0;
    ptr<const expr_list_t> l;
    ptr<const expr_dict_t> d;

    if ((l = to_list ())) { r = l->size (); }
    else if ((d = to_dict ())) { r = d->size (); }

    return r;
  }

  //-----------------------------------------------------------------------

  const_obj_t
  const_obj_t::operator[] (size_t i) const
  {
    ptr<const expr_list_t> v = to_list ();
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
      return const_obj_t (d->lookup (s));
    }
    return const_obj_t ();
  }

  //-----------------------------------------------------------------------

  ptr<const expr_list_t>
  const_obj_t::to_list () const
  {
    ptr<const expr_list_t> r;
    if (obj ()) r = obj ()->to_list ();
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_dict_t>
  const_obj_t::to_dict () const
  {
    ptr<const expr_dict_t> r;
    if (obj ()) r = obj ()->to_dict ();
    return r;
  }

  //=======================================================================

  obj_t
  obj_t::push_back ()
  {
    ptr<expr_list_t> v = to_list ();
    v->push_back ();
    obj_t o (obj_ref_list_t::alloc (v, v->size () - 1));
    return o;
  }

  //-----------------------------------------------------------------------

  void
  obj_t::push_back (obj_t o)
  {
    ptr<expr_list_t> v = to_list ();
    v->push_back (o.obj ());
  }

  //-----------------------------------------------------------------------

  obj_t
  obj_t::operator[] (size_t i)
  {
    ptr<expr_list_t> v = to_list ();
    return obj_t (obj_ref_list_t::alloc (v, i)); 
  }

  //-----------------------------------------------------------------------

  void
  obj_t::insert (const str &n, obj_t o)
  {
    ptr<expr_dict_t> d = to_dict ();
    d->replace (n, o.obj ());
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
  obj_t::to_list ()
  {
    if (!_list && obj ()) {
      _list = obj ()->to_list ();
    }

    if (!_list) {
      _scalar = NULL;
      _dict = NULL;
      _list = New refcounted<expr_list_t> ();
      update_value (_list);
    }
    return _list;
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
    _list = New refcounted<expr_list_t> ();
    _c_obj = _obj = _list;
  }


  //-----------------------------------------------------------------------

  ptr<expr_dict_t>
  obj_t::to_dict ()
  {
    if (!_dict && obj ()) {
      _dict = obj ()->to_dict ();
    }

    if (!_dict) {
      _scalar = NULL;
      _list = NULL;
      _dict = New refcounted<expr_dict_t> ();
      update_value (_dict);
    }
    return _dict;
  }

  //-----------------------------------------------------------------------

  obj_t &
  obj_t::set_value (ptr<expr_t> in)
  {
    if (!set_value_dict (in) && !set_value_list (in)) {
      set_value_scalar (in);
    }
    return (*this);
  }

  //----------------------------------------------------------------------

  void
  obj_t::clear_value ()
  {
    _list = NULL;
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
    _list =  NULL;
    _dict = NULL;
    _scalar = NULL;
  }

  //----------------------------------------------------------------------

  obj_t &
  obj_t::refer_to (obj_t in)
  {
    _ref = in._ref;
    _c_obj = _obj = in._obj;
    _list = in._list;
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
      _list = in._list;
      _dict = in._dict;
      _scalar = in._scalar;
    }
    return (*this);
  }

  //----------------------------------------------------------------------

  bool
  obj_t::set_value_scalar (ptr<expr_t> in)
  {
    bool ret = true;
    if (in) {
      _list = NULL;
      _dict = NULL;
      _scalar = in;
      update_value (_scalar);
    } else {
      ret = false;
    } 
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::set_value_list (ptr<expr_t> in)
  {
    bool ret = true;
    ptr<expr_list_t> v = in->to_list ();
    if (v) {
      _dict = NULL;
      _scalar = NULL;
      _list = v;
      update_value (_list);
    } else {
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  obj_t::set_value_dict (ptr<expr_t> in)
  {
    ptr<expr_dict_t> d = in->to_dict ();
    bool ret = true;
    if (d) {
      _list = NULL;
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
    _list = NULL;
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
    _list = NULL;
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
  obj_t::update_value (ptr<expr_t> v)
  {
    if (_ref) { _ref->set (v); }
    else      { _c_obj = _obj = v; }
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  obj_t::obj () const
  {
    if (_ref) { return _ref->get (); }
    else      { return _obj; }
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  obj_t::obj ()
  {
    if (_ref) { return _ref->get (); }
    else      { return _obj; }
  }

  //-----------------------------------------------------------------------

  ptr<expr_dict_t>
  obj_t::dict () 
  {
    ptr<expr_dict_t> d;
    if (_dict) d = _dict->to_dict ();
    return d;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_dict_t>
  obj_t::dict () const
  {
    ptr<const expr_dict_t> d;
    if (_dict) d = _dict->to_dict ();
    return d;
  }

  //-----------------------------------------------------------------------

  const_obj_t::const_obj_t (ptr<const obj_ref_t> r)
    : _c_obj (r ? r->get () : ptr<const expr_t> ())
  {}

  //-----------------------------------------------------------------------

  const_obj_t::const_obj_t (ptr<obj_ref_t> r)
    : _c_obj (r ? r->get () : ptr<expr_t> ())
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
    return (obj () && obj ()->to_int (i));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t> obj_t::expr () const { return obj (); }

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
    bool ret = false;
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
    return _c_obj && _c_obj->to_int (i);
  }

  //-----------------------------------------------------------------------

  bool
  const_obj_t::to_uint (u_int64_t *i) const
  {
    return _c_obj && _c_obj->to_uint (i);
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
    return _c_obj && (*s = _c_obj->to_str ());
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
    bool b = false;
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

  bool
  obj_t::append (obj_t in)
  {
    bool ret = true;
    ptr<expr_dict_t> d_me, d_in;
    ptr<expr_list_t> v_me, v_in;

    if ((d_me = to_dict ()) && (d_in = in.to_dict ())) {
      d_me->overwrite_with (*d_in);
    } else if ((v_me = to_list ()) && (v_in == in.to_list ())) {
      (*v_me) += (*v_in);
    } else {
      ret = false;
    }
    return ret;

  }

  //-----------------------------------------------------------------------

};
