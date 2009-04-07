#include "okrfn.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  rxx_fn_t::rxx_fn_t (const str &n, ptr<expr_list_t> l, int ln,
		      ptr<expr_regex_t> rxx,
		      ptr<expr_t> body,
		      ptr<expr_t> opts,
		      ptr<expr_t> val,
		      bool match)
    : predicate_t (n, l, ln),
      _rxx (rxx),
      _body (body),
      _opts (opts),
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
      body = (*e)[0];
      opts = (*e)[1];
      val = (*e)[2];
    } else if (narg == 2) {
      body = (*e)[0];
      val = (*e)[1];
    } else {
      *err = "match/search take 2 or 3 arguments";
      ok = false;
    }

    if ((x = body->to_regex_obj ())) {
      if (opts) {
	*err = "don't specify options when supplying a regex object!";
	ok = false;
      } else {
	body = NULL;
      }
    }

    if (ok) {
      ret = New refcounted<rxx_fn_t> (n, e, lineno, x, body, opts, val, match);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  rxx_fn_t::eval_internal_bool (eval_t e) const
  {
    ptr<rxx> x;
    if (_rxx) {
      x = _rxx->to_regex ();
    } else {
      str b = _body->eval_as_str (e);
      str o = _opts->eval_as_str (e);
      str err;

      if (!b) {
	report_error (e, "cannot evaluate regex in match/search");
      } else if (!(x = rxx_factory_t::compile (b, o, &err))) {
	report_error (e, err);
      }
    }

    str v = _val->eval_as_str (e);

    bool ret =  false;
    if (!x || !v) {
      /* noop */
    } else if (_match) {
      ret = x->match (v);
    } else {
      ret = x->search (v);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

};
