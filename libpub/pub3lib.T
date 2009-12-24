// -*-c++-*-

#include "pub3lib.h"

namespace pub3 {

  //================================== compiled_fn_t ======================

  compiled_fn_t::compiled_fn_t (str l, str n) : _lib (l), _name (n) {}

  //-----------------------------------------------------------------------

  str
  compiled_fn_t::to_str (bool q) const
  {
    str l = _lib;
    if (!l) l = "";
    strbuf b ("<compiled function> %s:%s", _lib.cstr (), _name.cstr ());
    return b;
  }

  //-----------------------------------------------------------------------

  tamed void
  compiled_fn_t::pub_args (publish_t *p, callable_t::args_t in, 
			   callable_t::cargs_t *out, 
			   evv_t ev) const
  {
    tvars {
      size_t i;
      ptr<const expr_t> x;
    }
    for (i = 0; i < in->size (); i++) {
      twait { (*in)[i]->pub_to_val (p, mkevent (x)); }
      if (!x && safe_args ()) x = expr_null_t::alloc ();
      out->push_back (x);
    }
    ev->trigger ();
  }

  //-----------------------------------------------------------------------

  void
  compiled_fn_t::eval_args (publish_t *p, args_t in, cargs_t *out) const
  {
    for (size_t i = 0; i < in->size (); i++) {
      ptr<const expr_t> x = (*in)[i]->eval_to_val (p);
      if (!x && safe_args ()) x = expr_null_t::alloc ();
      out->push_back (x);
    }
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  compiled_fn_t::eval_to_val (publish_t *p, args_t ai) const
  {
    cargs_t ao;
    eval_args (p, ai, &ao);
    ptr<const expr_t> ret = v_eval_1 (p, ao);
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void 
  compiled_fn_t::pub_to_val (publish_t *p, callable_t::args_t ai, 
			     cxev_t ev) const
  {
    tvars {
      callable_t::cargs_t ao;
      ptr<const expr_t> ret;
    }
    twait { pub_args (p, ai, &ao, mkevent ()); }
    ret = v_eval_1 (p, ao);
    ev->trigger (ret);
  }

  //===================================== patterned_fn_t ==================

  ptr<const expr_t>
  patterned_fn_t::v_eval_1 (publish_t *e, const cargs_t &args) const
  {
    ptr<const expr_t> ret;
    vec<arg_t> checked;
    if (check_args (e, args, &checked)) {
      ret = v_eval_2 (e, checked);
    }
    return ret;
  }

  //----------------------------------------------------------------------

  bool
  patterned_fn_t::check_args (publish_t *p, const cargs_t &args, 
			      vec<arg_t> *_checked) const
  {
    vec<str> err;
    
    const char *cp;
    size_t i = 0;
    bool pipe = false;

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
	if (ib && !(a._r = args[i]->to_regex ())) {
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

  //========================================== library_t ==================

  void
  library_t::bind (ptr<bindtab_t> t)
  {
    for (size_t i = 0; i < _functions.size (); i++) {
      ptr<compiled_fn_t> f = _functions[i];
      str n = f->name ();
      OKDBG3(PUB_RFN3, CHATTER, strbuf ("binding library function: ") << n);
      t->insert (n, f);
    }
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
