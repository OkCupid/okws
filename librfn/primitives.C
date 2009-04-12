
#include "okrfn.h"
#include "okformat.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  len_t::len_t (const str &n, ptr<expr_list_t> al, int ln, ptr<expr_t> a)
    : scalar_fn_t (n, al, ln), _arg (a) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  len_t::constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err)
  {
    ptr<runtime_fn_t> ret;
    ptr<expr_t> l, h;
    size_t narg = e ? e->size () : size_t (0);
    bool ok = true;

    if (narg != 1) {
      *err = "len () takes 1 argument";
      ok = false;
    }

    if (ok) {
      ret = New refcounted<len_t> (n, e, lineno, (*e)[0]);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  str
  predicate_t::eval_as_str (eval_t e) const
  {
    bool b = eval_internal_bool (e);
    return pub3::expr_bool_t::to_str (b);
  }

  //-----------------------------------------------------------------------

  scalar_obj_t
  predicate_t::eval_internal (eval_t e) const
  {
    bool b = eval_internal_bool (e);
    scalar_obj_t o;
    o.set_i (b);
    return o;
  }

  //-----------------------------------------------------------------------

  bool
  is_null_t::eval_internal_bool (eval_t e) const 
  {
    bool b = true;
    if (_arg) b = _arg->eval_as_null (e);
    return b;
  }

  //-----------------------------------------------------------------------

  scalar_obj_t
  len_t::eval_internal (eval_t e) const
  {
    ptr<const vec_iface_t> v;
    ptr<const expr_t> x;
    ptr<const pval_t> pv;
    size_t z = 0;

    if (!(pv = _arg->eval (e))) {
      report_error (e, "cannot evaluate argument to len()");
    } else if (!(x = pv->to_expr ())) {
      report_error (e, "arguments to len() must be pub3 objects");
    } else if (!x->to_len (&z)) {
      report_error (e, "bad argument type to len()");
    }

    scalar_obj_t o;
    o.set_u (z);
    return o;
  }

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
  
  ptr<runtime_fn_t>
  is_null_t::constructor (const str &n, ptr<expr_list_t> e, int ln, str *err)
  {
    size_t narg = e ? e->size () : size_t (0);
    ptr<runtime_fn_t> ret;
    if (narg == 1) {
      ret = New refcounted<is_null_t> (n, e, ln, (*e)[0]);
    } else {
      *err = "isnull() takes 1 argument";
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

  is_null_t::is_null_t (const str &n, ptr<expr_list_t> l, int lineno,
			ptr<expr_t> e)
    : predicate_t (n, l, lineno), _arg (e) {}

  //-----------------------------------------------------------------------


};
