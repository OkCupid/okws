
#include "okrfnlib.h"

namespace rfn1 {


  //-----------------------------------------------------------------------

  range_t::range_t (const str &n, ptr<expr_list_t> al, int ln,
		    ptr<expr_t> l, ptr<expr_t> h, ptr<expr_t> s)
    : runtime_fn_t (n, al, ln), _l (l), _h (h), _s (s) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  range_t::constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err)
  {
    ptr<runtime_fn_t> ret;
    ptr<expr_t> l, h, s;
    size_t narg = e ? e->size () : size_t (0);
    bool ok = true;

    if (narg == 0 || narg > 3) {
      ok = false;
      *err = "range() takes 1, 2 or 3 arguments";
    } else if (narg == 1) {
      h = (*e)[0];
    } else if (narg == 2) {
      l = (*e)[0];
      h = (*e)[1];
    } else if (narg == 3) {
      l = (*e)[0];
      h = (*e)[1];
      s = (*e)[2];
    }

    if (ok) {
      ret = New refcounted<range_t> (n, e, lineno, l, h, s);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<expr_list_t>
  range_t::eval_internal (eval_t e) const
  {
    int64_t s = 1;
    int64_t l = 0;
    int64_t h = 0;
    bool loud;

    loud = e.set_loud (true);
    if (_l) { l = _l->eval_as_int (e); }
    if (_s) { s = _s->eval_as_int (e); }
    h = _h->eval_as_uint (e);
    e.set_loud (loud);

    ptr<expr_list_t> el = New refcounted<expr_list_t> ();
    for (int64_t i = l; i < h; i += s) {
      el->push_back (expr_int_t::alloc (i));
    }
    return el;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  range_t::eval_freeze (eval_t e) const
  {
    return eval_internal (e);
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t>
  range_t::eval (eval_t e) const
  {
    return eval_internal (e);
  }

  //-----------------------------------------------------------------------

  join_t::join_t (const str &n, ptr<expr_list_t> el, int lineno)
    : scalar_fn_t (n, el, lineno),
      _join_str ((*el)[0]),
      _join_list ((*el)[1]) {}
  
  //-----------------------------------------------------------------------

  scalar_obj_t
  join_t::eval_internal (eval_t e) const
  {
    ptr<const vec_iface_t> l;
    str js;
    scalar_obj_t ret;

    if (!(l = _join_list->eval_as_vec (e))) {
      report_error (e, "cannot evaluate list argument to join");
    } else if (!(js = _join_str->eval_as_str (e))) {
      report_error (e, "cannot evaluate join string argument to join");
    } else {
      strbuf b;
      vec<str> hold;
      for (size_t i = 0; i < l->size (); i++) {
	if (i > 0) {
	  b << js;
	}
	str s;
	ptr<const pval_t> v = l->lookup (i);
	ptr<const expr_t> x;

	if (!v) {
	  /* nothing wanted, that's ok! */
	} else if (!(x = v->to_expr ())) {
	  strbuf err ("argument %zd in join list is not a pub3 obj!", i);
	  report_error (e, err);
	} else if (!(s = x->eval_as_str (e))) {
	  strbuf err ("cannot evaluate argument %zd in join list", i);
	  report_error (e, err);
	} else {
	  b << s;
	  hold.push_back (s);
	}
      }
      ret.set (str (b));
    }
    return ret;
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

};

