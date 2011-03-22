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

  static str 
  repl_wrapper (eval_t *p, ptr<const callable_t> fn, const vec<str> *matches)
  {
    ptr<expr_list_t> l = expr_list_t::alloc ();
    for (size_t i = 0; i < matches->size (); i++) {
      l->push_back (expr_str_t::safe_alloc ((*matches)[i]));
    }
    ptr<expr_list_t> args = expr_list_t::alloc ();
    args->push_back (l);
    ptr<const expr_t> res = fn->eval_to_val (p, args);
    str out;
    if (res) { out = res->to_str (); }
    return out;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  replace_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str input = args[0]._s;
    ptr<rxx> pat = args[1]._r;
    ptr<const pub3::expr_t> r = args[2]._O;
    bool use_repl_2 = true;
    if (args.size () == 4) {
      use_repl_2 = args[3]._b;
    }

    ptr<expr_t> ret;
    str str_repl;
    ptr<const callable_t> fn_repl;
    str s;

    if (!r) { /* noop */ }
    else if ((fn_repl = r->to_callable ())) {
      s = rxx_replace (input, *pat, wrap (repl_wrapper, p, fn_repl));
    } else if ((str_repl = r->to_str ())) {
      if (use_repl_2) { s = rxx_replace_2 (input, *pat, str_repl); }
      else            { s = rxx_replace   (input, *pat, str_repl); }
    } else {
      report_error (p, "replace argument is neither a string or a function");
    }
    ret = expr_str_t::safe_alloc (s);
    return ret;
  }

  //-----------------------------------------------------------------------
};

