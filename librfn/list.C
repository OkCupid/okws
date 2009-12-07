
#include "okrfnlib.h"

namespace rfn1 {


  //-----------------------------------------------------------------------

  ptr<const expr_t>
  range_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    int64_t s = args.size () >= 3 ? args[2]._i : 1;
    int64_t l = args.size () >= 2 ? args[0]._i : 0;
    int64_t h = args.size () >= 2 ? args[1]._i : _args[0]._i;

    ptr<expr_list_t> el = New refcounted<expr_list_t> ();
    for (int64_t i = l; i < h; i += s) {
      el->push_back (expr_int_t::alloc (i));
    }
    return el;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  join_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<const vec_iface_t> l;
    str js;
    scalar_obj_t ret;

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

  tamed void
  append_t::pub_to_val (publish_t *p, args_t args, cvex_t ev) const
  {
    tvars {
      ptr<mref_t> r;
      ptr<expr_t> x;
      ptr<expr_list_t> l;
      ptr<const expr_t> cel;
      ptr<expr_t> el;
    }
    if (args->size () < 2) {
      report_error (e, "append() takes 2 or more arguments");
    } else {
      twait { (*args)[0]->pub_to_ref (p, mkevent (r)); }
    }

    if (!r || !(x = r->get_value ())) {
      report_error (e, "first argument to append() must be non-null");
    } else if (!(l = x->to_list ())) {
      report_error (e, "first argument to append() must be a list");
    } else {
      for (size_t i = 1; i < args->size (); i++) {
	twait { (*args)[i]->pub_to_val (p, mkevent (cel)); }
	if (cel) el = cel->copy ();
	if (!el) el = expr_null_t::alloc ();
	l->push_back (el);
      }
    }
    ev->trigger (l);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  map_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_dict_t> d = args[0]._d;
    ptr<const expr_t> x = args[1]._O;

    return eval_internal (p, d, x);
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  map_t::eval_internal (publish_t *p, ptr<const expr_dict_t> m, 
			ptr<const expr_t> x) const
  {
    ptr<const expr_list_t> l;
    ptr<const expr_dict_t> d;
    str s;
    ptr<expr_t> ret;

    if ((l = x->to_list ())) {
      ptr<expr_list_t> out = New refcounted<expr_list_t> ();
      for (size_t i = 0; i < l->size (); i++) {
	ptr<const expr_t> cx = (*l)[i];
	ptr<const expr_t> cn = eval_interal (p, m, cx);
	ptr<expr_t> n;
	if (cn) n = cn->copy ();
	if (!n) n = expr_null_t::alloc ();
	out->push_back (n);
      }
      ret = out;
    } else if ((d = x->to_dict ())) {
      bintab_t::const_iterator_t it (*id);
      ptr<expr_dict_t> d = expr_dict_t::alloc ();
      str *k;
      ptr<expr_t> cx;
      while ((k = it.next (&cx))) {
	ptr<const expr_t> cn = eval_interal (p, m, cx);
	ptr<expr_t> n;
	if (cn) n = cn->copy ();
	if (!n) n = expr_null_t::alloc ();
	d->insert (*k, n);
      }
      ret = d;
    } else if ((s = x->to_str (false))) {
      ptr<const expr_t> *cxp = m->lookup (s);
      ptr<const expr_t> cx;
      if (cxp) { cx = *cxp; }
      if (cx) { ret = cx->copy (); }
    } else if (x->is_null ()) {
      /* noop */
    } else {
      report_error (e, "second argument to map() must be a vec, "
		    "dict or string");
    }
    if (!ret) { ret = expr_null_t::alloc (); }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  split_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
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
};

