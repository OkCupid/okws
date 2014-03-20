// -*-c++-*-

#include "okrfn-int.h"
#include "pubutil.h"
#include "pescape.h"
#include "okcgi.h"
#include "crypt.h"
#include "wide_str.h"
#include "okws_rxx.h"

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
  str wss_filter_t::filter (str s) { return html_wss (s); }

  //------------------------------------------------------------

  const str toupper_t::DOCUMENTATION =
    "Convert all alpha characters in //s// to uppercase";

  const str tolower_t::DOCUMENTATION =
    "Convert all alpha characters in //s// to lowercase";

  const str html_escape_t::DOCUMENTATION =
    "HTML escapes these chars: & < >";

  const str url_escape_t::DOCUMENTATION =
    "Produce a URL-encoded version of the string //s//";

  const str url_unescape_t::DOCUMENTATION =
    "Take a URL-escaped string //s// and return the unescaping of it.";

  const str utf8_fix_t::DOCUMENTATION =
    "Strips out any malformed UTF-8 characters from //s// "
    "and returns the fixed string.";

  const str hidden_escape_t::DOCUMENTATION =
    "HTML escapes these chars: \' \" & < >";

  const str wss_filter_t::DOCUMENTATION =
    "compacts multiple adjacent whitespace characters into one";

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

  const str tag_escape_t::DOCUMENTATION =
    "Escape all tags except for <b>, <i>, <br> and <p>.";

  //------------------------------------------------------------

  ptr<const expr_t>
  substr_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    size_t start = 0, len = 0;
    start = args[1]._i;
    size_t slen = utf8_len (s);

    if (args.size () >= 3) { len = args[2]._i; }
    else { len = slen; }

    if (start >= slen) { start = slen; }
    if (start + len >= slen) { len = slen - start; }

    str r = utf8_substr (s, start, len);
    return expr_str_t::safe_alloc (r);
  }

  //------------------------------------------------------------

  const str substr_t::DOCUMENTATION =
    "Output a substring of //s//, starting at "
    "character //start// and //len// bytes long.";

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

  const str default_t::DOCUMENTATION =
    "Given the input value //in//, check to see if it's "
    "//null.//  If //null//, then output //val//, outherwise, output //in//. "
    "If //val// wasn't specified and //in// is null, then output the empty "
    "string ''";

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
      if (!is_empty (v[i].cstr())) {
	if (output) { b << " "; }
	b << v[i];
	output = true;
      }
    }
    return b;
  }

  //------------------------------------------------------------

  const str strip_t::DOCUMENTATION =
    "Strips leading, trailing spaces and reduce in-between "
    "spaces to just one space";

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

  const str sha1_t::DOCUMENTATION =
     "Produce the SHA-1 hash of //s//, outputting a "
      "hexidecimal-encoded digest.";

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

  const str format_float_t::DOCUMENTATION =
    "Format the float //val// accoring to the given sprintf-style "
    "format string //fmt//.";

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

  const str format_int_t::DOCUMENTATION =
    "Format the int //val// accoring to the given sprintf-style "
    "format string //fmt//.";

  //------------------------------------------------------------

  ptr<const expr_t>
  json_escape_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    str ret = json_escape (s, true, p->utf8_json ());
    return expr_str_t::alloc (ret);
  }

  //------------------------------------------------------------

  const str json_escape_t::DOCUMENTATION =
    "Escapes all double-quotes, backslashes, newlines and tabs.";

  ptr<const expr_t>
  js_escape_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str s = args[0]._s;
    str ret = json_escape (s, true, p->utf8_json ());
    ret = rxx_replace_2(ret, "</", "<\\/");
    ret = rxx_replace_2(ret, "'", "\\'");
    return expr_str_t::alloc (ret);
  }

  //------------------------------------------------------------

  const str js_escape_t::DOCUMENTATION =
    "First runs json_escape, then replaces \"</\" with \"<\\/\".";

  //------------------------------------------------------------

  ptr<const expr_t>
  index_of_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str big_s = args[0]._s;
    str small_s = args[1]._s;
    size_t start = 0;
    if (args.size () == 3) { start = args[2]._i; }
    int ret = -1;
    if (start < big_s.len ()) {
      const char *big = big_s + start;
      const char *small = small_s.cstr();
      const char *pos = strstr (big, small);
      if (pos) { ret = (pos - big) + start; }
    }
    return expr_int_t::alloc (ret);
  }

  //------------------------------------------------------------

};
