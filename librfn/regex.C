#include "okrfn.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  rxx_fn_t::rxx_fn_t (const str &n, ptr<expr_list_t> l, int ln,
		      ptr<expr_t> val,
		      bool match)
    : predicate_t (n, l, ln),
      _val (val),
      _match (match) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  rxx_fn_t::constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err)
  {
    ptr<runtime_fn_t> ret;
    size_t narg = e ? e->size () : size_t (0);
    bool ok = true;
    ptr<expr_regex_t> x;
    ptr<expr_t> body, opts, val;
    bool match;

    match = (n == "match");

    if (narg == 3) {
      val = (*e)[2];
    } else if (narg == 2) {
      val = (*e)[1];
    } else {
      *err = "match/search take 2 or 3 arguments";
      ok = false;
    }

    if (ok) {
      ret = New refcounted<rxx_fn_t> (n, e, lineno, val, match);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  rxx_fn_t::eval_internal_bool (eval_t e) const
  {
    ptr<rxx> x;
    str v;
    bool ret = false;

    if (!(x = (*_arglist)[0]->eval_as_regex (e, false)) &&
	!(x = _arglist->eval_as_regex (e, true))) {
      report_error (e, "cannot evaluate regex in match/search");
    } else if (!_val || !(v = _val->eval_as_str (e))) {
      /* noop -- false */
    } else if (_match) {
      ret = x->match (v);
    } else {
      ret = x->search (v);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

};
