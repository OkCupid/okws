#include "okrfnlib.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  import_t::import_t (const str &n, ptr<expr_list_t> al, int ln)
    : runtime_fn_t (n, al, ln) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  import_t::constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err)
  {
    ptr<runtime_fn_t> ret;
    if (!e || e->size () == 0) {
      *err = "import() takes 1 or more arguments";
    } else {
      ret = New refcounted<import_t> (n, e, lineno);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  import_t::eval_internal (eval_t e) const
  {
    ptr<const expr_t> x;
    ptr<const aarr_t> d;
    penv_t *env;
    
    bool res = true;

    for (size_t i = 0; _arglist && i < _arglist->size (); i++) {
      bool ok = false;
      if (!(x = (*_arglist)[i])) {
	report_error (e, strbuf ("empty arg (%zu) in import()", i));
      } else if (!(d = x->eval_as_dict (e))) {
	report_error (e, strbuf ("cannot evalute arg %zu in import()", i));
      } else if (!(env = e.penv ())) {
	report_error (e, "empty env in import()");
      } else {
	env->safe_push (d);
	ok = true;
      }
      if (!ok) res = false;
    }
    return expr_bool_t::alloc (res);
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t>
  import_t::eval (eval_t e) const 
  { return eval_internal (e); }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  import_t::eval_freeze (eval_t e) const 
  { return eval_internal (e); }

  //-----------------------------------------------------------------------

};
