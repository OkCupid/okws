#include "okrfn.h"
#include "okws_rxx.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  regex_fn_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str target, opts, body;
    ptr<const expr_t> ret;
    body = args[0]._s;
    if (args.size () == 2) {
      target = args[1]._s;
    } else {
      target = args[2]._s;
      opts = args[1]._s;
    }
    ptr<rxx> x = str2rxx (p, body, opts);
    if (!x) {
      report_error (p, "cannot parse regular expression");
    } else {
      bool b = match() ? x->match (target) : x->search (target);
      ret = expr_bool_t::alloc (b);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  replace2_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str input = args[0]._s;
    ptr<rxx> pat = args[1]._r;
    str repl = args[2]._s;

    return expr_str_t::safe_alloc (rxx_replace_2 (input, *pat, repl));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  replace_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str input = args[0]._s;
    ptr<rxx> pat = args[1]._r;
    str repl = args[2]._s;

    return expr_str_t::safe_alloc (rxx_replace (input, *pat, repl));
  }

  //-----------------------------------------------------------------------
};

