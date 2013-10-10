// -*-c++-*-

#include "pub3lib.h"

namespace pub3 {

  //================================== compiled_fn_t ======================

  compiled_fn_t::compiled_fn_t (str l, str n) : _lib (l), _name (n) {}

  //-----------------------------------------------------------------------

  str
  compiled_fn_t::to_str (str_opt_t o) const
  {
    str l = _lib;
    if (!l) l = "";
    strbuf b ("<compiled function> %s:%s", l.cstr (), _name.cstr ());
    return json::quote (b, o);
  }

  //-----------------------------------------------------------------------

  void
  compiled_fn_t::set_name (str l, str n)
  {
    if (l) _lib = l;
    if (n) _name = n;
  }

  //-----------------------------------------------------------------------

  tamed void
  compiled_fn_t::pub_args (eval_t *p, callable_t::args_t in, 
			   callable_t::margs_t *out, 
			   evv_t ev) const
  {
    tvars {
      size_t i;
      ptr<expr_t> x;
    }
    for (i = 0; i < in->size (); i++) {
      twait { (*in)[i]->pub_to_mval (p, mkevent (x)); }
      if (!x && safe_args ()) x = expr_null_t::alloc ();
      out->push_back (x);
    }
    ev->trigger ();
  }

  //-----------------------------------------------------------------------

  void
  compiled_fn_t::eval_args (eval_t *e, args_t in, margs_t *out) const
  {
    for (size_t i = 0; i < in->size (); i++) {
      ptr<const expr_t> a = (*in)[i];
      ptr<expr_t> x = a->eval_to_mval (e);
      if (!x && safe_args ()) x = expr_null_t::alloc ();
      out->push_back (x);
    }
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  compiled_fn_t::eval_to_val (eval_t *e, args_t ai) const
  {
    margs_t ao;
    eval_args (e, ai, &ao);
    ptr<const expr_t> ret = v_eval_1 (e, ao);
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void 
  compiled_fn_t::pub_to_val (eval_t *p, callable_t::args_t ai, 
			     cxev_t ev) const
  {
    tvars {
      callable_t::margs_t ao;
      ptr<const expr_t> ret;
    }
    twait { pub_args (p, ai, &ao, mkevent ()); }
    ret = v_eval_1 (p, ao);
    ev->trigger (ret);
  }

  //================================ patterned_fn_base_t ===================

  bool
  patterned_fn_base_t::check_args (eval_t *p, const margs_t &args, 
				   vec<arg_t> *_checked) const
  {
    vec<str> err;
    
    const char *cp;
    size_t i = 0;
    bool pipe = false;
    str e;
    int64_t tmp;

    size_t min_args = 0, max_args = 0;

    for (cp = _arg_pat; *cp ; cp++) {
      arg_t a;
      bool ib = i < args.size (); 

      bool adv = true;

      // only advance in the arg is in bounds.  
      // and then, maybe it won't work.
      bool push = ib;

      size_t hi = i + 1; // argument index for humans

      if (ib && safe_args ()) assert (args[i]);

      switch (*cp) {

      case 'F':
	if (ib && !(a._F = args[i]->to_callable ())) {
	  strbuf b ("argument %zd: expected a lambda or built-in", hi);
	  err.push_back (b);
	}
	break;
	
      case 'i':
	if (ib && !args[i]->to_int (&a._i)) {
	  strbuf b ("argument %zd: expected an integer", hi);
	  err.push_back (b);
	}
	break;
	
      case 'u': 
	if (ib && !args[i]->to_uint (&a._u)) {
	  strbuf b ("argument %zd: expected an unsigned int", hi);
	  err.push_back (b);
	}
	break;

      case 'f':
	if (!ib) { /* noop */ }
	else if (args[i]->to_double (&a._f)) { /* good */ }
	else if (args[i]->to_int (&tmp)) { a._f = tmp; }
	else {
	  strbuf b ("argument %zd: expected a double (or int)", hi);
	  err.push_back (b);
	}
	break;
	
      case 's': 
	if (ib && !(a._s = args[i]->arg_to_str ())) {
	  strbuf b ("argument %zd: expected a string", hi);
	  err.push_back (b);
	}
	break;
	
      case 'O':  
	if (ib) {
	  a._O = args[i];
	}
	break;
	
      case 'r':
	if (ib && !(a._r = args[i]->to_regex (&e))) {
	  if (e) err.push_back (e);
	  strbuf b ("argument %zd: expected a regex", hi);
	  err.push_back (b);
	}
	break;
	
      case 'd':
	if (ib && !(a._d = args[i]->to_dict ())) {
	  strbuf b ("argument %zd: expected a dict", hi);
	  err.push_back (b);
	}
	break;

      case 'b':
	if (ib) {
	  a._b = args[i]->to_bool ();
	}
	break;
	
      case 'l':
	if (ib && !(a._l = args[i]->to_list ())) {
	  strbuf b ("argument %zd: expected a list", hi);
	  err.push_back (b);
	}
	break;
	
      case '|':
	if (pipe) {
	  err.push_back ("internal pattern error: "
			 "'|' can only be specified once");
	} else {
	  pipe = true;
	}
	adv = push = false;
	break;
	
      default:
	err.push_back (strbuf ("internal pattern error: bad character: %c", 
			       *cp));
	adv = push = false;
	break;
      }
    
      if (adv) {
	max_args ++;
	if (!pipe) min_args ++;
	i++;
      }
      if (push) {
	_checked->push_back (a);
      }
    }
    

    if (args.size () < min_args || args.size () > max_args) {
      str ex;
      if (min_args == max_args) {
	ex = strbuf ("%zd", min_args);
      } else {
	ex = strbuf ("%zd to %zd", min_args, max_args);
      }
      strbuf e ("wrong number of arguments: expected %s, got %zd", 
		ex.cstr (), args.size ());
      err.push_back (e);
    }

    for (size_t e = 0; e < err.size (); e++) {
      strbuf b ("%s: %s", _name.cstr (), err[e].cstr ());
      report_error (p, b);
    }

    return (err.size () == 0);
  }

  //===================================== patterned_fn_t ==================

  ptr<const expr_t>
  patterned_fn_t::v_eval_1 (eval_t *e, const margs_t &args) const
  {
    ptr<const expr_t> ret;
    vec<arg_t> checked;
    if (check_args (e, args, &checked)) {
      ret = v_eval_2 (e, checked);
    }
    return ret;
  }

  //===================================== patterned_fn_t ==================

  tamed void
  patterned_fn_blocking_t::pub_to_val (eval_t *e, callable_t::args_t ai, 
				       cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      vec<patterned_fn_base_t::arg_t> ac;
      callable_t::margs_t ao;
    }

    // Three steps:
    //  args_in (ai) to args_out (ao) via pub_args
    //  args_out (ao) to args_checked (ac) via check_args
    //  args_checks (ac) to result via virtual pub hook.

    twait { pub_args (e, ai, &ao, mkevent ()); }
    if (check_args (e, ao, &ac)) {
      twait { v_pub_to_val_2 (e, ac, mkevent (ret)); }
    }
    ev->trigger (ret);
  }

  //========================================== library_t ==================

  void
  library_t::bind (ptr<bindtab_t> t)
  {
    for (size_t i = 0; i < _functions.size (); i++) {
      ptr<compiled_fn_t> f = _functions[i];
      str n = f->name ();
      OKDBG3(PUB_RFN3, CHATTER, strbuf ("binding library function: ") << n);
      if (!_only_in_libname) { t->insert (n, f); }
      if (_all_dict) { _all_dict->insert (n, f); }
    }
    if (_all_dict && _all_libname) { t->insert (_all_libname, _all_dict); }
  }

  //-----------------------------------------------------------------------

  void library_t::bind_all (str s, bool only_in_libname) {
    _all_libname = s;
    if (_doc) {
      // TODO: do we want to use the recycler here?
      _all_dict = New refcounted<documented<expr_dict_t>>(_doc);
    } else {
      _all_dict = expr_dict_t::alloc ();
    }
    _only_in_libname = only_in_libname;
  }

  //-----------------------------------------------------------------------

  ptr<bindtab_t>
  library_t::bind ()
  {
    ptr<bindtab_t> t = bindtab_t::alloc ();
    bind (t);
    return t;
  }

  //-----------------------------------------------------------------------

  void library_t::import (ptr<library_t> l)
  { singleton_t::get ()->import (l->bind ()); }

  //-----------------------------------------------------------------------

  void library_t::clear_all ()
  { singleton_t::get ()->libraries ()->clear (); }

  //=======================================================================

  const char *okws_version_str () { return OKWS_VERSION; }

  u_int64_t okws_version_int () 
  { return VERSION_FLATTEN (OKWS_VERSION_MAJOR, OKWS_VERSION_MINOR, 
			    OKWS_VERSION_PATCHLEVEL, OKWS_VERSION_PRE); }

  //=======================================================================

};
