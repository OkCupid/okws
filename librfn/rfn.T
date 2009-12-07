// -*-c++-*-

#include "okrfn.h"

namespace rfn3 {

  //================================== compiled_fn_t ======================

  compiled_fn_t::compiled_fn_t (str n, str lib) : _name (n) {}

  //-----------------------------------------------------------------------
  
  void complied_fn_t::set_lib (str l) { _lib = l; }
  
  //-----------------------------------------------------------------------

  str
  compiled_fn_t::to_str () const
  {
    str l = _lib;
    if (!l) l = "";
    strbuf b ("<compiled function> %s:%s", _lib.cstr (), _name.cstr ());
    return b;
  }

  //---------------------------------------------------------------------

  bool
  compiled_fn_t::check_args (publish_t *p, args_t a) const
  {
    size_t n = a->size ();
    bool ret = true;
    if (n < min_args () || n > max_args ()) {
      ret = false;
      str e;
      if (min_args () == max_args ()) {
	e = strbuf ("%zd", min_args ());
      } else {
	e = strbuf ("%zd to %zd", min_args (), max_args ());
      }
      strbuf b ("Wrong number of parameters to call %s "
		"(expected %s, got %zu)", _name.cstr (), e.cstr (), n);
      report_error (p, b);
    }
    return ret;
  }
  
  //-----------------------------------------------------------------------

  tamed void
  compiled_fn_t::pub_args (publish_t *p, args_t in, event<args_t>::ref ev) const
  {
    tvars {
      args_t out (expr_list_t::alloc ());
      size_t i;
      ptr<const expr_t> x;
    }
    for (i = 0; args && i < in->size (); i++) {
      twait { (*in)[i]->pub_to_val (p, mkevent (x)); }
      out->push_back (x);
    }
    ev->trigger (out);
  }

  //-----------------------------------------------------------------------

  args_t
  compile_fn_t::eval_args (publish_t *p, args_t in) const
  {
    args_t out = expr_list_t::alloc ();
    for (size_t i = 0; i < in->size (); i++) {
      ptr<const expr_t> x = eval_to_val (p, (*in)[i]);
      out->push_back (x);
    }
    return out;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  compiled_fn_t::eval_to_val (publish_t *p, args_t ai) const
  {
    args_t ao = eval_args (p, ai);
    ptr<const expr_t> ret = v_eval_1 (p, ao);
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void 
  compiled_fn_t::pub_to_val (publish_t *p, args_t ai, cxev_t ev) const
  {
    tvars {
      args_t ao;
      ptr<const expr_t> ret;
    }
    twait { pub_args (p, ai, mkevent (ao)); }
    ret = v_eval_1 (p, ao);
    ev->trigger (ret);
  }

  //===================================== patterned_fn_t ==================

  ptr<const expr_t>
  patterned_fn_t::v_eval_1 (publish_t *e, args_t args) const
  {
    ptr<const expr_t> ret;
    vec<arg_t> checked;
    if (check_args (*args, &checked)) {
      ret = v_eval_2 (e, checked);
    }
    return ret;
  }

  //----------------------------------------------------------------------

  bool
  patterned_fn_t::check_args (const expr_list_t &args, vec<arg_t> *_checked)
  {
    vec<str> err;
    
    const char *cp = _arg_pat;
    size_t i = 0;
    bool pipe = false;

    size_t min_len = 0;
    size_t max_len = 0;
    
    for ( ; *cp && i < args.size (); cp++) {
      bool adv = true;
      args_t a;

      switch (*cp) {
	
      case 'i':
	if (!args[i]->to_int (&a._i)) {
	  strbuf b ("argument %zd: expected an integer", i);
	  err.push_back (b);
	}
	break;
	
      case 'u':
	u_int64_t du;
	if (!args[i]->to_uint (&a._u)) {
	  strbuf b ("argument %zd: expected an unsigned int", i);
	  err.push_back (b);
	}
	break;

      case 's': 
	if (!(a._s = args[i]->to_str ())) {
	  strbuf b ("argument %zd: expected a string", i);
	  err.push_back (b);
	}
	break;

      case 'O':  
	a._O = args[i];
	break;

      case 'r':
	if (!(a._r = args[i]->to_regex ())) {
	  strbuf b ("argument %zd: expected a regex", i);
	  err.push_back (b);
	}
	break;

      case 'd':
	if (!(a._d = args[i]->to_dict ())) {
	  strbuf b ("argument %zd: expected a dict", i);
	  err.push_back (b);
	}
	break;

      case 'l':
	if (!(a._l = args[i]->to_list ())) {
	  strbuf b ("argument %zd: expected a list", i);
	  err.push_back (b);
	}
	break;

      case '|':
	if (pipe) {
	  err.push_back ("internal pattern error: "
			 "'|' can only be specified once");
	  pok = false;
	} else {
	  pipe = true;
	}
	adv = false;
	break;

      default:
	err.push_back (strbuf ("internal pattern error: bad character: %c", 
			       *cp));
	adv = false;
	break;
      }

      if (adv) {
	max_size ++;
	if (!pipe) min_size ++;
	i++;
      }
      _checked->push_back (a);
    }

    if (*cp || i < args.size ()) {
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
      strbuf b ("%s: %s", _name.cstr (), e.cstr ());
      report_error (p, b);
    }

    return (e.size () == 0);
  }

  //=======================================================================
  //=======================================================================
  //=======================================================================
  //=======================================================================

};
