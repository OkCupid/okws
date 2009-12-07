
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

  append_t::append_t (const str &n, ptr<expr_list_t> el, int lineno)
    : runtime_fn_t (n, el, lineno),
      _list ((*el)[0]) {}

  //-----------------------------------------------------------------------
  
  ptr<runtime_fn_t>
  append_t::constructor (const str &n, ptr<expr_list_t> el, int lineno,
			    str *err)
  {
    ptr<runtime_fn_t> r;
    if (el->size () < 2) {
      *err = "append() takes two are more arguments";
    } else {
      r = New refcounted<append_t> (n, el, lineno);
    }
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t> append_t::eval (eval_t e) const 
  { return eval_internal (e); }

  //-----------------------------------------------------------------------

  ptr<pval_t> append_t::eval_freeze (eval_t e) const 
  { return eval_internal (e); }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  append_t::eval_internal (eval_t e) const
  {
    ptr<expr_ref_t> rf;
    ptr<slot_ref_t> slot;
    ptr<expr_t> x;
    ptr<expr_list_t> outlist;
    ptr<pval_t> v;

    eval_t lhs_eval = e;

    if (!(rf = _list->to_ref ())) {
      report_error (e, "first argument to append must be a reference");
    } else if (!(slot = rf->lhs_deref (&lhs_eval))) {
      report_error (e, "first argument to append was not settable");
    } else if (!(x = slot->deref_expr ()) || !(v = x->eval_freeze (lhs_eval))) {
      outlist = New refcounted<expr_list_t> (rf->lineno ());
    } else if (!(outlist = v->to_expr_list ())) {
      report_error (e, "first argument to append must be a list");
    } 
    
    if (outlist && rf) {
      for (size_t i = 1; i < _arglist->size (); i++) {
	ptr<expr_t> in = (*_arglist)[i];
	ptr<expr_t> out;
	if (!in || !(v = in->eval_freeze (e)) || !(out = v->to_expr ())) {
	  out = expr_null_t::alloc (rf->lineno ());
	}
	outlist->push_back (out);
      }
      slot->set_expr (outlist);
    }
    return expr_null_t::alloc (rf ? rf->lineno () : -1);
  }

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  map_t::constructor (const str &n, ptr<expr_list_t> e, int lineno,
		      str *err)
  {
    bool ok = true;
    size_t narg = e ? e->size () : size_t (0);
    ptr<runtime_fn_t> ret;
    ptr<expr_t> d, l;

    if (narg != 2) {
      ok = false;
      *err = "map() takes 2 arguments; a dict and a list";
    } else {
      d = (*e)[0];
      l = (*e)[1];
    }

    if (ok) {
      ret = New refcounted<map_t> (n, e, lineno, d, l);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  map_t::map_t (const str &n, ptr<expr_list_t> el, int lineno,
		ptr<expr_t> d, ptr<expr_t> l)
    : runtime_fn_t (n, el, lineno),
      _map (d), _list (l) {}

  //-----------------------------------------------------------------------

  ptr<expr_t>
  map_t::eval_internal (eval_t e) const
  {
    ptr<expr_t> ret;
    ptr<const aarr_t> m;

    if (!(m = _map->eval_as_dict (e))) {
      report_error (e, "first argument to map() must be a dict");
    } else {
      ret = eval_internal (e, m, _list);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t> map_t::eval (eval_t e) const { return eval_internal (e); }
  ptr<pval_t> map_t::eval_freeze (eval_t e) const { return eval_internal (e); }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  map_t::eval_internal (eval_t e, ptr<const aarr_t> mp, 
			ptr<const expr_t> arg) const
  {
    str k;
    ptr<expr_t> ret;
    ptr<const vec_iface_t> targ_v;
    ptr<const aarr_t> targ_d;
      
    if (arg->eval_as_vec_or_dict (e, &targ_v, &targ_d)) {

      // Handle the case in which the arg is a vector.
      if (targ_v) {
	ptr<expr_list_t> out = New refcounted<expr_list_t> ();
	for (size_t i = 0; i < targ_v->size (); i++) {
	  ptr<const pval_t> v;
	  ptr<const expr_t> cx;
	  ptr<expr_t> x;
	  if ((v = targ_v->lookup (i)) && (cx = v->to_expr ())) {
	    x = eval_internal (e, mp, cx);
	  }
	  if (!x) { x = expr_null_t::alloc (); }
	  out->push_back (x);
	}
	ret = out;

      } else {
	// Handle the case in which the arg is a dict
	const nvtab_t *nvt = targ_d->nvtab ();
	ptr<expr_dict_t> out = New refcounted<expr_dict_t> ();
	for (nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
	  ptr<const pval_t> v;
	  ptr<const expr_t> cx;
	  ptr<expr_t> x;
	  if ((v = p->value_ptr ()) && (cx = v->to_expr ())) {
	    x = eval_internal (e, mp, cx);
	  }
	  if (!x) { x = expr_null_t::alloc (); }
	  out->to_dict ()->replace (p->name (), x);
	}
	ret = out;
      }

    } else if ((k = arg->eval_as_str (e))) {
      ptr<const pval_t> cv;
      ptr<pval_t> v;
      ptr<const expr_t> cx;

      // need to eval scalars at the end of the day, since the dict might
      // have live expressions in it --- we never froze the dict!
      if ((cv = mp->lookup_ptr (k)) && 
	  (cx = cv->to_expr ()) &&
	  (v = cx->eval_freeze (e))) {
	ret = v->to_expr ();
      }

    } else if (arg->eval_as_null (e)) {
      /* noop */
    } else {
      report_error (e, "second argument to map() must be a vec, "
		    "dict or string");
    }

    if (!ret) {
      ret = expr_null_t::alloc ();
    }

    return ret;
  }

  //-----------------------------------------------------------------------

  split_t::split_t (const str &n, ptr<expr_list_t> el, int lineno,
		    ptr<expr_t> r, ptr<expr_t> v)
    : runtime_fn_t (n, el, lineno), _regex (r), _val (v) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  split_t::constructor (const str &n, ptr<expr_list_t> el, int lineno, str *err)
  {
    size_t narg = el ? el->size () : size_t (0);
    ptr<runtime_fn_t> ret;
    if (narg != 2) {
      *err = "split() takes two arguments: a regex and string";
    } else {
      ret = New refcounted<split_t> (n, el, lineno, (*el)[0], (*el)[1]);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<expr_list_t>
  split_t::eval_internal (eval_t e) const
  {
    ptr<expr_list_t> ret;
    ptr<expr_regex_t> ex;
    ptr<rxx> rx;
    str s;

    if (!_regex || !(rx = _regex->eval_as_regex (e))) {
      report_error (e, "cannot evaluate first arg to split() as a regex");
    } else if (!_val || !(s = _val->eval_as_str (e))) {
      report_error (e, "cannot evaluate second arg to split() as a string");
    } else {
      ret = New refcounted<expr_list_t> ();
      vec<str> v;
      split (&v, *rx, s);
      for (size_t i = 0; i < v.size (); i++) {
	ptr<expr_t> e;
	if (v[i]) e = New refcounted<expr_str_t> (v[i]);
	else e = expr_null_t::alloc ();
	ret->push_back (e);
      }
    }
    return ret;

  }

  //-----------------------------------------------------------------------

  ptr<pval_t> split_t::eval_freeze (eval_t e) const 
  { return eval_internal (e); }
  
  //-----------------------------------------------------------------------

  ptr<const pval_t> split_t::eval (eval_t e) const { return eval_internal (e); }

  //-----------------------------------------------------------------------
};

