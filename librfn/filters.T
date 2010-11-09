// -*-c++-*-

#include "okrfn.h"
#include "pescape.h"
#include "okcgi.h"
#include "crypt.h"
#include "wide_str.h"

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

  str toupper_t::filter (str s) { return my_toupper (s); }
  str tolower_t::filter (str s) { return my_tolower (s); }
  str html_escape_t::filter (str s) { return xss_escape (s); }
  str hidden_escape_t::filter (str s) { return htmlspecialchars (s); }
  str url_escape_t::filter (str s) { return cgi_encode (s); }
  str url_unescape_t::filter (str s) { return cgi_decode (s);  }
  str utf8_fix_t::filter (str s) { return utf8_fix (s); }

  //------------------------------------------------------------

  ptr<const expr_t>
  tag_escape_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> r;

    if (args.size () >= 2) {
      r = args[1]._r;
    } else { 
      static ptr<rxx> dflt_rxx;
      if (!(r = dflt_rxx)) {
	str x = "<[/\\s]*(b|br|i|p)[/\\s]*>";
	str err;
	r = pub3::rxx_factory_t::compile (x, "i", &err);
	if (!r) {
	  report_error (p, err);
	} else {
	  dflt_rxx = r;
	}
      }
    }

    str s;
    if (r) {
      html_filter_rxx_t filt (r);
      str in = args[0]._s;
      s = filt.run (in);
    }
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  substr_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
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

  bool
  default_t::count_args (eval_t *p, size_t sz) const
  {
    bool ret = true;
    if (sz < 1 || sz > 2) {
      report_error (p, "default() expects 1 or 2 args");
      ret = false;
    }
    return ret;
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  default_t::eval_to_val (eval_t *p, args_t args) const
  {
    ptr<const expr_t> ret, def;
    bool is_null;

    if (count_args (p, args->size ())) {
     
      is_null = (*args)[0] ? (*args)[0]->eval_as_null (p) : true;
      
      if (args->size () > 1) { def = (*args)[1]->eval_to_val (p); } 
      else { def = expr_str_t::alloc (""); }
      
      ret = is_null ? def : (*args)[0]->eval_to_val (p);
    }
    return ret;
  }

  //------------------------------------------------------------

  tamed void
  default_t::pub_to_val (eval_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret, def;
      bool is_null;
    }

    if (count_args (p, args->size ())) {
      if (!(*args)[0]) { 
	is_null = true; 
      } else { 
	twait { (*args)[0]->pub_as_null (p, mkevent (is_null)); } 
      }

      if (args->size () > 1) { 
	twait { (*args)[1]->pub_to_val (p, mkevent (def)); }
      } else { 
	def =  expr_str_t::alloc ("");
      }

      if (is_null) { 
	ret = def; 
      } else { 
	twait { (*args)[0]->pub_to_val (p, mkevent (ret)); }
      }
    }
    ev->trigger (ret);
  }

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

  str strip_t::filter (str s)
  {
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
    return b;
  }

  //------------------------------------------------------------

  str sha1_t::filter (str s) 
  {
    char buf[sha1::hashsize];
    sha1_hash (buf, s.cstr (), s.len ());
    strbuf b;
    b << hexdump (buf, sha1::hashsize);
    return b;
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  format_float_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
#define BUFSZ 1024
    char buf[BUFSZ];
    str fmt = args[0]._s;
    double d = args[1]._f;
    snprintf (buf, BUFSZ, fmt.cstr (), d);
    return expr_str_t::alloc (buf);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  format_int_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    char buf[BUFSZ];
    str fmt = args[0]._s;
    int i = args[1]._i;
    snprintf (buf, BUFSZ, fmt.cstr (), i);
    return expr_str_t::alloc (buf);
#undef BUFSZ
  }


  //------------------------------------------------------------

  ptr<const expr_t>
  json_escape_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    str ret = json_escape (s, true, p->utf8_json ());
    return expr_str_t::alloc (ret);
  }

  //------------------------------------------------------------

};
