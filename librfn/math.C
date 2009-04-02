
#include "okrfn.h"
#include "okformat.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  random_t::random_t (const str &n, ptr<expr_list_t> al, int ln, 
		      ptr<expr_t> l, ptr<expr_t> h)
    : scalar_fn_t (n, al, ln), _low (l), _high (h) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  random_t::constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err)
  {
    ptr<runtime_fn_t> ret;
    ptr<expr_t> l, h;
    size_t narg = e ? e->size () : size_t (0);
    bool ok = true;

    if (narg == 0 || narg > 2) {
      ok = false;
      *err = "random() takes 1 or 2 arguments";
    } else if (narg == 1) {
      h = (*e)[0];
    } else {
      l = (*e)[0];
      h = (*e)[1];
    }

    if (ok) {
      ret = New refcounted<random_t> (n, e, lineno, l, h);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t>
  scalar_fn_t::eval (eval_t e) const
  {
    return expr_t::alloc (eval_internal (e));
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  scalar_fn_t::eval_freeze (eval_t e) const
  {
    return expr_t::alloc (eval_internal (e));
  }

  //-----------------------------------------------------------------------

  scalar_obj_t
  random_t::eval_internal (eval_t e) const
  {
    u_int64_t def_range = 10;
    u_int64_t l = 0;
    u_int64_t h = def_range;
    bool loud;

    loud = e.set_loud (true);
    if (_low) {
      l = _low->eval_as_uint (e);
    }
    h = _high->eval_as_uint (e);
    e.set_loud (loud);

    int64_t d = h - l;

    if (d <= 0) {
      strbuf b ("range for random must be greater than 0 (got %" PRId64 ")", d);
      report_error (e, b);
      h = l + def_range;
    }

    int64_t range = h - l;
    assert (range > 0);

    u_int64_t v = (random () % range) + l;
    scalar_obj_t o;
    o.set_u (v);
    return o;
  }

  //-----------------------------------------------------------------------


};
