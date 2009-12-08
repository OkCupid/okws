// -*-c++-*-

#include "okrfn.h"
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
    s = my_toupper (s);
    return expr_str_t::safe_alloc (s);
  }
  
  //------------------------------------------------------------

  ptr<const expr_t>
  tolower_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    s = my_tolower (s);
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  html_escape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    s = xss_escape (s);
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  tag_escape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
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

  tamed void
  default_t::pub_to_val (publish_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      scalar_obj_t ret;
      bool is_null;
      scalar_obj_t def;
      ptr<expr_t> er;
    }
    if (args->size () < 1 || args->size () > 2) {
      report_error (p, "default() expects 1 or 2 args");

    } else {
     
      if (!(*args)[0]) { 
	is_null = true; 
      } else { 
	twait { (*args)[0]->pub_as_null (p, mkevent (is_null)); } 
      }

      if (args->size () > 1) { 
	twait { (*args)[1]->pub_as_scalar (p, mkevent (def)); }
      } else { 
	def.set (""); 
      }

      if (is_null) { 
	ret = def; 
      } else { 
	twait { (*args)[0]->pub_as_scalar (p, mkevent (ret)); }
      }
    }
    er = expr_t::alloc (ret);
    ev->trigger (er);
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

  ptr<const expr_t>
  strip_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
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
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  hidden_escape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    s = htmlspecialchars (s);
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  url_escape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    s = cgi_encode (s);
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  url_unescape_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    s = cgi_decode (s);
    return expr_str_t::safe_alloc (s);
  }

  //------------------------------------------------------------

  ptr<const expr_t>
  sha1_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    char buf[sha1::hashsize];
    sha1_hash (buf, s.cstr (), s.len ());
    strbuf b;
    b << hexdump (buf, sha1::hashsize);
    return expr_str_t::safe_alloc (b);
  }

  //------------------------------------------------------------

};
