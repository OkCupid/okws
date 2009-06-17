
#include "okrfn.h"
#include "pescape.h"

namespace rfn1 {
  
  //------------------------------------------------------------

  static str
  my_toupper (const str &in)
  {
    if (!in) return in;
    
    mstr out (in.len ());
    for (size_t i = 0; i < in.len (); i++) {
      out[i] = toupper (in[i]);
    }
    out.setlen (in.len ());
    return out;
  }
  
  //------------------------------------------------------------

  tolower_t::tolower_t (const str &nm, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (nm, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t
  tolower_t::eval_internal (eval_t e) const
  {
    str s;
    if (_arg) { s = _arg->eval_as_str (e); }
    if (s) { s = my_tolower (s); }
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------

  toupper_t::toupper_t (const str &nm, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (nm, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t
  toupper_t::eval_internal (eval_t e) const
  {
    str s;
    if (_arg) { s = _arg->eval_as_str (e); }
    if (s) { s = my_toupper (s); }
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------

  html_escape_t::html_escape_t (const str &nm, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (nm, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t
  html_escape_t::eval_internal (eval_t e) const
  {
    str s;
    if (_arg) { s = _arg->eval_as_str (e); }
    if (s) { s = xss_escape (s); }
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------

  json_escape_t::json_escape_t (const str &nm, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (nm, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t
  json_escape_t::eval_internal (eval_t e) const
  {
    str s;
    if (_arg) { s = _arg->eval_as_str (e); }
    if (!s) { s = ""; }
    s = json_escape (s, true);
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------

  substring_t::substring_t (const str &nm, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (nm, e, lineno), 
      _arg ((*e)[0]),
      _start ((*e)[1]),
      _len ((*e)[2]) {}

  //------------------------------------------------------------

  scalar_obj_t
  substring_t::eval_internal (eval_t e) const
  {
    str s;

    if (_arg) { s = _arg->eval_as_str (e); }
    size_t start = 0, len = 0;
    if (_start) { start = _start->eval_as_int (e); }
    if (_len) { len = _len->eval_as_int (e); }

    if (!s) { s = ""; } 
    if (start >= s.len ()) { start = s.len (); }
    if (start + len >= s.len ()) { len = s.len () - start; }

    str r = str (s.cstr () + start, len);
    return scalar_obj_t (r);
  }

  //------------------------------------------------------------

  default_t::default_t (const str &nm, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (nm, e, lineno), 
      _arg ((*e)[0])
  {
    if (e->size () > 1) {
      _def_val = (*e)[1];
    }
  }

  //------------------------------------------------------------

  scalar_obj_t
  default_t::eval_internal (eval_t e) const
  {
    scalar_obj_t ret;

    bool isnull = !_arg || _arg->eval_as_null (e);
    if (isnull) {
      if (_def_val) { 
	str v = _def_val->eval_as_str (e); 
	ret.set (v);
      } else { 
	ret.set ("");
      }
    } else {
      ret = _arg->eval_as_scalar (e);
    }

    return ret;
  }

  //------------------------------------------------------------

  strip_t::strip_t (const str &nm, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (nm, e, lineno), 
      _arg ((*e)[0]) {}

  //------------------------------------------------------------

  static bool
  is_empty (const char *s)
  {
    for (const char *cp = s; *cp; cp++) {
      if (!isspace (*cp)) return false;
    }
    return true;
  }


  //------------------------------------------------------------

  scalar_obj_t
  strip_t::eval_internal (eval_t e) const
  {
    scalar_obj_t ret;
    str s = _arg->eval_as_str (e);
    if (s) {
      static rxx x ("\\s+");
      vec<str> v;
      split (&v, x, s);
      strbuf b;
      bool output = false;
      for (size_t i = 0; i < v.size (); i++) {
	if (!is_empty (v[i])) {
	  if (output) { b << " "; }
	  b << v[i];
	  output = true;
	}
      }
      s = b;
    } else {
      s = "";
    }
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------
};
