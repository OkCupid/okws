#include "okrfnlib.h"
#include "pescape.h"
#include "okcgi.h"
#include "crypt.h"

namespace rfn3 {
  
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

  ptr<const expr_t>
  toupper_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    if (s) { s = my_toupper (s); }
    return expr_str_t::safe_alloc (s);
  }
  
  //------------------------------------------------------------

  ptr<const expr_t>
  tolower_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    if (s) { s= my_tolower (s); }
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  html_escape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    if (s) { s = xss_escape (s); }
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  tag_escape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> p;

    if (args.size () >= 2) {
      p = args[1]._r;
    } else { 
      static ptr<rxx> dflt_rxx;
      if (!(p = dflt_rxx)) {
	str x = "<[/\\s]*(b|br|i|p)[/\\s]*>";
	str err;
	p = pub3::rxx_factory_t::compile (x, "i", &err);
	if (!p) {
	  report_error (e, err);
	}
	dflt_rxx = p;
      }
    }

    str s;
    if (p) {
      html_filter_rxx_t filt (p);
      str in = args[0]._s;
      s = filt.run (in);
    }
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  json_escape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    s = json_escape (s, true);
    return expr_str_t::safe_alloc (s);
  }
  
  //------------------------------------------------------------

  ptr<const expr_t>
  substring_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    if (!s) s = "";
    size_t start = 0, len = 0;
    start = args[1]._i;

    if (args.size () >= 3) { len = args[2]._i; }
    else { len = s.len (); }

    if (start >= s.len ()) { start = s.len (); }
    if (start + len >= s.len ()) { len = s.len () - start; }

    str r = str (s.cstr () + start, len);
    return expr_str_t::safe_alloc (r);
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

  hidden_escape_t::hidden_escape_t (const str &nm, ptr<expr_list_t> e, 
				    int lineno)
    : scalar_fn_t (nm, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t
  hidden_escape_t::eval_internal (eval_t e) const
  {
    str s;
    if (_arg) { s = _arg->eval_as_str (e); }
    if (!s) { s = ""; }
    s = htmlspecialchars (s);
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------

  url_escape_t::url_escape_t (const str &n, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (n, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t 
  url_escape_t::eval_internal (eval_t e) const
  {
    str s = _arg->eval_as_str (e);
    if (!s) {
      report_error (e, "cannot evaluate arg to url_escape() as a string");
      s = "";
    } else if (s.len () == 0) {
      s = "";
    } else {
      s = cgi_encode (s);
    }
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------

  url_unescape_t::url_unescape_t (const str &n, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (n, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t 
  url_unescape_t::eval_internal (eval_t e) const
  {
    str s = _arg->eval_as_str (e);
    if (!s) {
      report_error (e, "cannot evaluate arg to url_unescape() as a string");
      s = "";
    } else if (s.len () == 0) {
      s = "";
    } else {
      s = cgi_decode (s);
    }
    return scalar_obj_t (s);
  }

  //------------------------------------------------------------

  sha1_t::sha1_t (const str &n, ptr<expr_list_t> e, int lineno)
    : scalar_fn_t (n, e, lineno), _arg ((*e)[0]) {}

  //------------------------------------------------------------

  scalar_obj_t
  sha1_t::eval_internal (eval_t e) const
  {
    str s = _arg->eval_as_str (e);
    str ret;
    if (!s) {
      report_error (e, "cannot evaluate arg to sha() as a string");
      ret = "";
    } else {
      char buf[sha1::hashsize];
      sha1_hash (buf, s.cstr (), s.len ());
      strbuf b;
      b << hexdump (buf, sha1::hashsize);
      ret = b;
    }
    return scalar_obj_t (ret);
  }

  //------------------------------------------------------------

};
