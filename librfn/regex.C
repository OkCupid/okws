// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */


#include "okrfn-int.h"
#include "okws_rxx.h"

//-----------------------------------------------------------------------

// Strip all occurrences of the character 'c' from the input string in
static str
stripchar (str in, char c)
{
  mstr out (in.len ()+1);
  const char *inp = in.cstr();
  char *outp = out;
  for ( ; *inp; inp++) {
    if (*inp != c) { *(outp++) = *inp; }
  }
  *outp = '\0';
  out.setlen (outp - out.cstr ());
  return out;
}

//-----------------------------------------------------------------------

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  regex_fn_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str target, opts, body;
    ptr<const expr_t> ret;
    bool tuple_out = false;
    body = args[0]._s;

    if (args.size () > 1) {
      target = args[1]._s;
    }

    if (args.size () > 2) {
      opts = args[2]._s;
    }

    if ((tuple_out = (opts && strchr (opts.cstr (), 'T')))) {
      opts = stripchar (opts, 'T');
    }

    ptr<rxx> x = str2rxx (p, body, opts);
    if (!x) {
      report_error (p, "cannot parse regular expression");
    } else {

      bool b = match() ? x->match (target) : x->search (target);

      // In the case of a 'T' argument, passed then we return as tuples
      // the match groups.
      if (tuple_out) {

          ptr<expr_list_t> l = expr_list_t::alloc ();
          if (b) {
              str s;
              size_t i (0);
              while ((s = x->at(i++))) { l->push_back (expr_str_t::alloc (s)); }
          }
          ret = l;

      } else {
          // Otherwise, let's return a bool, true for a match, and false
          // for a lack of match.
          ret = expr_bool_t::alloc (b);
      }

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

