// -*-c++-*-

#include "okrfn.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  enumerate_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> out = expr_list_t::alloc ();
    ptr<const expr_list_t> in = args[0]._l;
    size_t len = in ? in->size () : size_t (0);
    for (size_t i = 0; i < len; i++) {
      ptr<expr_list_t> tuple = expr_list_t::alloc ();
      tuple->push_back (expr_uint_t::alloc (i));
      tuple->push_back (safe_copy ((*in)[i]));
      out->push_back (tuple);
    }
    return out;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  decorate_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> out = expr_list_t::alloc ();
    ptr<const expr_list_t> in = args[0]._l;
    size_t len = in ? in->size () : size_t (0);
    for (size_t i = 0; i < len; i++) {
      ptr<const expr_t> x = (*in)[i];
      ptr<const expr_dict_t> xd;
      ptr<expr_t> el;
      if (!x) { el = expr_null_t::alloc (); }
      else if ((xd = x->to_dict ())) {
	ptr<expr_dict_t> nxd = xd->copy_dict ();
	nxd->insert ("iter", expr_int_t::alloc (i));
	nxd->insert ("odd", expr_bool_t::alloc (i % 2 != 0));
	nxd->insert ("even", expr_bool_t::alloc (i % 2 == 0));
	nxd->insert ("count", expr_int_t::alloc (len));
	nxd->insert ("first", expr_bool_t::alloc (i == 0));
	nxd->insert ("last", expr_bool_t::alloc (len - 1 == i));
	el = nxd;
      }
      out->push_back (el);
    }
    return out;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  range_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    int64_t s = args.size () >= 3 ? args[2]._i : 1;
    int64_t l = args.size () >= 2 ? args[0]._i : 0;
    int64_t h = args.size () >= 2 ? args[1]._i : args[0]._i;

    ptr<expr_list_t> el = New refcounted<expr_list_t> ();
    for (int64_t i = l; i < h; i += s) {
      el->push_back (expr_int_t::alloc (i));
    }
    return el;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  join_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    strbuf b;
    vec<str> hold;
    ptr<const expr_list_t> l = args[1]._l;
    str js = args[0]._s;

    for (size_t i = 0; i < l->size (); i++) {
      if (i > 0) {
	b << js;
      }

      str s;
      ptr<const expr_t> x = (*l)[i];

      if (!x || x->is_null ()) { s = ""; }
      else s = x->to_str (false);
      b << s;
      hold.push_back (s);
    }
    return expr_str_t::safe_alloc (b);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  append_t::v_eval_1 (eval_t *p, const margs_t &args) const
  {
    ptr<expr_t> x;
    ptr<expr_list_t> l;
    ptr<const expr_t> cel;
    ptr<expr_t> el;
    size_t i;

    if (args.size () < 2) {
      report_error (p, "append() takes 2 or more arguments");
    } else if (!(x = args[0])) {
      report_error (p, "first argument to append() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (p, "first argument to append() must be a list");
    } else {
      for (i = 1; i < args.size (); i++) {
	  cel = args[i];
	  if (cel) el = cel->copy ();
	  if (!el) el = expr_null_t::alloc ();
	  l->push_back (el);
	}
    }
    return l;
  }

  //-----------------------------------------------------------------------
  
  ptr<const expr_t>
  map_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_dict_t> d = args[0]._d;
    ptr<const expr_t> x = args[1]._O;

    return eval_internal (p, d, x);
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  map_t::eval_internal (eval_t *p, ptr<const expr_dict_t> m, 
			ptr<const expr_t> x) const
  {
    ptr<const expr_list_t> l;
    ptr<const expr_dict_t> cd;
    str s;
    ptr<expr_t> ret;

    if ((l = x->to_list ())) {
      ptr<expr_list_t> out = New refcounted<expr_list_t> ();
      for (size_t i = 0; i < l->size (); i++) {
	ptr<const expr_t> cx = (*l)[i];
	ptr<const expr_t> cn = eval_internal (p, m, cx);
	ptr<expr_t> n;
	if (cn) n = cn->copy ();
	if (!n) n = expr_null_t::alloc ();
	out->push_back (n);
      }
      ret = out;
    } else if ((cd = x->to_dict ())) {
      bindtab_t::const_iterator_t it (*cd);
      ptr<expr_dict_t> d = expr_dict_t::alloc ();
      const str *k;
      ptr<expr_t> cx;
      while ((k = it.next (&cx))) {
	ptr<const expr_t> cn = eval_internal (p, m, cx);
	ptr<expr_t> n;
	if (cn) n = cn->copy ();
	if (!n) n = expr_null_t::alloc ();
	d->insert (*k, n);
      }
      ret = d;
    } else if ((s = x->to_str (false))) {
      ptr<const expr_t> cx = m->lookup (s);
      if (cx) { ret = cx->copy (); }
    } else if (x->is_null ()) {
      /* noop */
    } else {
      report_error (p, "second argument to map() must be a vec, "
		    "dict or string");
    }
    if (!ret) { ret = expr_null_t::alloc (); }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  split_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> rx = args[0]._r;
    str s = args[1]._s;
    ptr<expr_list_t> ret = New refcounted<expr_list_t> ();
    vec<str> v;
    split (&v, *rx, s);
    for (size_t i = 0; i < v.size (); i++) {
      ptr<expr_t> e;
      if (v[i]) e = New refcounted<expr_str_t> (v[i]);
      else e = expr_null_t::alloc ();
      ret->push_back (e);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  contains_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_list_t> l = args[0]._l;
    ptr<const expr_t> o = args[1]._O;
    bool ret = false;
    for (size_t i = 0; !ret && i < l->size (); i++) {
      ptr<const expr_t> val = (*l)[i];
      if (val) { ret = expr_EQ_t::eval_static (val, o); }
    }
    return expr_bool_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  tamed void
  pop_front_t::pub_to_mval (eval_t *p, callable_t::args_t args, 
			    xev_t ev) const
  {
    tvars {
      ptr<expr_t> x;
      ptr<expr_t> ret;
    }

    if (count_args (p, args->size ())) {
      twait { (*args)[0]->pub_to_mval (p, mkevent (x)); }
      ret = eval_final (p, x);
    }

    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  ptr<expr_t> 
  pop_front_t::eval_final (eval_t *p, ptr<expr_t> x) const
  {
    ptr<expr_t> ret;
    ptr<expr_list_t> l;
    if (!x) {
      report_error (p, "first argument to pop_front() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (p, "first argument to pop_front() must be a list");
    } else if (l->size ()) {
      ret = l->pop_front ();
    } else {
      ret = expr_null_t::alloc ();
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  pop_front_t::pub_to_val (eval_t *p, callable_t::args_t args, 
			   cxev_t ev) const
  {
    tvars {
      ptr<expr_t> out;
    }
    twait { pub_to_mval (p, args, mkevent (out)); }
    ev->trigger (out);
  }

  //-----------------------------------------------------------------------

  tamed void
  pop_front_t::pub_to_ref (eval_t *p, callable_t::args_t args, 
			   mrev_t ev) const
  {
    tvars {
      ptr<expr_t> out;
      ptr<mref_t> r;
    }
    twait { pub_to_mval (p, args, mkevent (out)); }
    if (out) { r = const_mref_t::alloc (out); }
    ev->trigger (r);
  }

  //-----------------------------------------------------------------------

  ptr<mref_t> 
  pop_front_t::eval_to_ref (eval_t *p, args_t args) const
  {
    ptr<mref_t> ret;
    ptr<expr_t> x = eval_to_mval (p, args);
    if (x) { ret = const_mref_t::alloc (x); }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t> pop_front_t::eval_to_val (eval_t *p, args_t args) const
  { return eval_to_mval (p, args); }

  //-----------------------------------------------------------------------

  bool
  pop_front_t::count_args (eval_t *p, size_t s) const
  {
    bool ret = true;
    if (s != 1) {
      ret = false;
      report_error (p, "pop_front() takes one argument");
    }
    return ret;
  }

  //-----------------------------------------------------------------------
  
  ptr<expr_t> 
  pop_front_t::eval_to_mval (eval_t *p, args_t args) const
  {
    ptr<expr_t> x , ret;

    if (count_args (p, args->size ())) {
      x = (*args)[0]->eval_to_mval (p); 
      ret = eval_final (p, x);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

};

