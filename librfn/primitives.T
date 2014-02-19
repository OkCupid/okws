// -*-c++-*-

#include "okrfn-int.h"
#include "okformat.h"
#include "pub3out.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  bool
  isnull_t::count_args (eval_t *p, size_t sz) const
  {
    bool ret = true;
    if (sz != 1) {
      ret = false;
      report_error (p, "isnull() takes 1 argument");
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  isnull_t::pub_to_val (eval_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      bool b (true);
    }
    if (count_args (p, args->size ())) {
      twait { (*args)[0]->pub_as_null (p, mkevent (b)); }
    }
    ret = expr_bool_t::alloc (b);
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  isnull_t::eval_to_val (eval_t *p, callable_t::args_t args) const
  {
    bool b = true;
    if (count_args (p, args->size ())) { b = (*args)[0]->eval_as_null (p); }
    ptr<const expr_t> ret = expr_bool_t::alloc (b);
    return ret;
  }

  //-----------------------------------------------------------------------

  const str isnull_t::DOCUMENTATION =
    "Output //True// if //o// is //null// and false otherwise.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  len_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    int ret = -1;
    size_t l; 
    if (!args[0]._O->to_len (&l)) {
      report_error (p, "object doesn't support the len() call");
    } else {
      ret = l;
    }
    return expr_int_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  const str len_t::DOCUMENTATION =
    "Given an input object //o//, returns its length. "
    "If //o// is a string, return the length of //s//.  If it is a list, "
    "return the number of elements in the list.  If it is a dictionary, "
    "return the number of bound pairs in the dictionary.";
  
  //-----------------------------------------------------------------------

  ptr<const expr_t>
  type_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str ret;
    ptr<const expr_t> x;
    if (!(x = args[0]._O)) {
      ret = "undef";
    } else { 
      ret = x->type_to_str ();
    }
    return expr_str_t::alloc (ret);
  }
  //-----------------------------------------------------------------------

  const str type_t::DOCUMENTATION =
    "Given object //o//, output its type. Possible values are undef, list, "
    "str, dict.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  version_str_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    return expr_str_t::alloc (version_str ());
  }

  //------------------------------------------------------------------------

  const str version_str_t::DOCUMENTATION =
    "Output the version of this rfn3 library in dotted string notation.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  version_int_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  { 
    return expr_uint_t::alloc (version_int ());
  }

  //-----------------------------------------------------------------------

  const str version_int_t::DOCUMENTATION =
    "Output the version of this rfn3 library in integer representation.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  okws_version_str_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  { return expr_str_t::alloc (okws_version_str ()); }

  //-----------------------------------------------------------------------

  const str okws_version_str_t::DOCUMENTATION =
    "Output the version of this OKWS instance in dotted string notation.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  okws_version_int_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  { return expr_uint_t::alloc (okws_version_int ()); }

  //-----------------------------------------------------------------------

  const str okws_version_int_t::DOCUMENTATION =
    "Output the version of this OKWS instance in integer representation.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  dump_env_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> l = p->env ()->to_list ();
    return l;
  }

  //-------------------------------------------------------------------

  const str dump_env_t::DOCUMENTATION =
    "Return a dump of the environment; useful for debugging purposes.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  int_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const pub3::expr_t> e = args[0]._O;
    int64_t i = 0;
    double d;
    if (!e) { /* nothing */ }
    else if (e->to_int (&i)) { /* great! */ }
    else if (e->to_double (&d)) { i = int (d); }
    else { /* no dice! */ }
    return expr_int_t::alloc (i);
  }

  //-----------------------------------------------------------------------

  const str int_t::DOCUMENTATION =
    "Coerces the object //o// into an integer, if at all possible.  Returns 0 "
    "on failure and the conversion otherwise.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  uint_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const pub3::expr_t> e = args[0]._O;
    uint64_t i = 0;
    double d;
    if (!e) { /* nothing */ }
    else if (e->to_uint (&i)) { /* great! */ }
    else if (e->to_double (&d)) { i = uint64_t (d); }
    else { /* no dice! */ }
    return expr_uint_t::alloc (i);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  documentation_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const pub3::expr_t> e = args[0]._O;
    // Not really sure why this can be null but
    // str_t::v_eval_2 below handles that case. Might as well stray on the safe
    // side.
    const str *s = e? e->documentation() : NULL;
    if (s) {
      return expr_str_t::safe_alloc (*s);
    } else {
      return expr_null_t::alloc ();
    }
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  str_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const pub3::expr_t> e = args[0]._O;
    str s;
    if (e) s = e->to_str (false);
    return expr_str_t::safe_alloc (s);
  }

  //-----------------------------------------------------------------------

  const str str_t::DOCUMENTATION =
    "Return the stringified version of the object //o//.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  logwarn_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_t> ret = expr_null_t::alloc ();
    str s = args[0]._s;
    bool use_prfx = args.size () > 1 ? args[1]._b : true;
    static rxx eolx ("\n");
    static rxx charx ("\\S");
    vec<str> v;
    split (&v, eolx, s);

    str prfx;
    if (use_prfx) { prfx = "okws-pub3[logwarn]: "; } 
    else { prfx = ""; }

    for (size_t i = 0; i < v.size (); i++) {
      if (charx.search (v[i])) {
	warn << prfx << v[i] << "\n";
      }
    }
    return ret;
  }

   //-----------------------------------------------------------------------

  const str logwarn_t::DOCUMENTATION =
    "Output the string //s// as an error to the log (not to "
    "the page); prefix with okws-pub3[logwarn] if the specified flag is true "
    "(and it is true by default).";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  warn_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_t> ret = expr_null_t::alloc ();
    str s = args[0]._s;
    if (!s) s = "";
    report_error (p, s);
    return ret;
  }

  //-----------------------------------------------------------------------

  const str warn_t::DOCUMENTATION =
     "Output the string //s// as an error, both to the page "
     "and to the log.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  warn_trace_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_t> ret = expr_null_t::alloc ();
    str s = args[0]._s;
    if (!s) s = "";
    p->output_err_stacktrace (s, P_ERR_EVAL);
    return ret;
  }

  //-----------------------------------------------------------------------

  const str warn_trace_t::DOCUMENTATION =
       "Output the string //s// and the current include "
       "stacktrace as an error, both to the page and to the log.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  internal_dump_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    dumper_t d;
    args[0]._O->dump (&d);
    strbuf b;
    d.dump_to (b);
    return expr_str_t::alloc (b);
  }

  //-----------------------------------------------------------------------

  const str internal_dump_t::DOCUMENTATION =
            "Return the okws-pub3 internals of the object //o//";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  enable_wss_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    bool ret = p->out ()->enable_wss (args[0]._b);
    return expr_bool_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  const str enable_wss_t::DOCUMENTATION =
    "Set white-space-stripping mode to //b//, either "
    "enabling it or disabled it.  WSS is off by default.";

  //-----------------------------------------------------------------------

  static bool
  str_to_scope (str s, env_t::layer_type_t *outp)
  {
    bool ok = true;
    env_t::layer_type_t out = env_t::LAYER_GLOBALS;
    if (!s) {}
    else if (s == "l") { out = env_t::LAYER_LOCALS; }
    else if (s == "u") { out = env_t::LAYER_UNIVERSALS; }
    else if (s == "g") {}
    else { ok = false; }
    *outp = out;
    return ok;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  bind_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str key, scope_str;
    ptr<pub3::expr_t> o;
    env_t::layer_type_t scope (env_t::LAYER_GLOBALS);
    bool found (false);
    ptr<bindtab_t> layer;

    key = args[0]._s;
    o = args[1]._O;
    if (args.size () == 3 && !str_to_scope (args[2]._s, &scope)) {
      report_error (p, strbuf ("bad scope specifier given: '%s'", 
			       args[2]._s.cstr ()));
				 
    } else {
      if (!o) { o = expr_null_t::alloc (); }
      layer = p->env ()->lookup_layer (key, scope, true);
      if (layer) {
	found = true;
	layer->insert (key, o);
      }
    }
    return expr_bool_t::alloc (found);
  }

  //-----------------------------------------------------------------------

  const str bind_t::DOCUMENTATION =
    "Bind the object //obj// to the key //key// in the "
    "environment. If //scope// is not specified, make it a local binding. "
    "Otherwise, can specify the scope explicitly: 'l' for 'local', 'g' for "
    "'global', or 'u' for 'universals.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  unbind_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str key = args[0]._s;
    env_t::layer_type_t scope = env_t::LAYER_GLOBALS;
    bool found = false;
    if (args.size () > 1 && !str_to_scope (args[1]._s, &scope)) {
      report_error (p, strbuf ("bad scope specifier given: '%s'", 
			       args[1]._s.cstr ()));
    } else {
      ptr<bindtab_t> layer = p->env ()->lookup_layer (key, scope, true);
      if (layer && (*layer)[key]) {
	found = true;
	layer->remove (key);
      }
    }
    return expr_bool_t::alloc (found);
  }

  //-----------------------------------------------------------------------

  const str unbind_t::DOCUMENTATION =
    "Unbind the variable //var// from the environment. "
    "If //scope// is not specified, then assume first found binding.  Can "
    "explicitly specify 'l' for 'local', 'g' for 'global', or 'u' "
    "for 'universals'.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  copy_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    return args[0]._O->copy ();
  }

  //-----------------------------------------------------------------------

  const str copy_t::DOCUMENTATION =
    "Return a copy of the input object //o//.";

  //-----------------------------------------------------------------------

  tamed void
  lookup_t::pub_to_ref (eval_t *p, callable_t::args_t args, mrev_t ev) const
  {
    tvars {
      str key, scope_str;
      ptr<mref_t> ret;
    }
    if (count_args (p, args->size ())) {
      twait { (*args)[0]->pub_as_str (p, mkevent (key)); }
      if (args->size () == 2) {
	twait { (*args)[1]->pub_as_str (p, mkevent (scope_str)); }
      }
      ret = eval_final (p, key, scope_str);
    }
    ev->trigger (ret);
  }
    
  //-----------------------------------------------------------------------

  ptr<mref_t>
  lookup_t::eval_final (eval_t *p, str key, str scope_str) const
  {
    env_t::layer_type_t scope (env_t::LAYER_GLOBALS);
    ptr<bindtab_t> layer;
    ptr<mref_t> ret;

    if (!key) {
      report_error (p, "cannot resolve key to lookup()");
    } else if (scope_str && !str_to_scope (scope_str, &scope)) {
      report_error (p, strbuf ("bad scope specifier given: '%s'",
			       scope_str.cstr ()));
    } else if (scope_str) {
      layer = p->env ()->lookup_layer (key, scope, false);
      if (layer) {
	ret = New refcounted<mref_dict_t> (layer, key);
      }
    } else {
      ret = p->env ()->lookup_ref (key);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  lookup_t::pub_to_val (eval_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      ptr<mref_t> r;
      ptr<const expr_t> out;
    }
    twait { pub_to_ref (p, args, mkevent (r)); }
    if (r) { out = r->get_value (); }
    ev->trigger (out);
  }

  //-----------------------------------------------------------------------

  bool
  lookup_t::count_args (eval_t *p, size_t s) const
  {
    bool ret = true;
    if (s != 1 && s != 2) {
      report_error (p, "lookup() takes 1 or 2 arguments");
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<mref_t>
  lookup_t::eval_to_ref (eval_t *p, args_t args) const
  {
    str key, scope_str;
    ptr<mref_t> ret;
    if (count_args (p, args->size ())) {
      key = (*args)[0]->eval_as_str (p);
      if (args->size () == 2) {
	scope_str = (*args)[1]->eval_as_str (p);
      }
      ret = eval_final (p, key, scope_str);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  lookup_t::eval_to_val (eval_t *p, args_t args) const
  {
    ptr<mref_t> r = eval_to_ref (p, args);
    ptr<const expr_t> out;
    if (r) { out = r->get_value (); }
    return out;
  }

  //-----------------------------------------------------------------------

  const str lookup_t::DOCUMENTATION =
    "Lookup the key //k// in the environment. "
    "If no //scope// is specified, find the first natural result. "
    "Otherwise, limit the search to the scope given by //scope//. "
    "See //unbind// or //bind// for how to specify scope.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  eval_location_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    pub3::obj_t o;
    p->get_loc_stack ()->back ().pub (o);
    return o.obj ();
  }
  
  //-----------------------------------------------------------------------

  ptr<const expr_t>
  breadcrumb_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    const runloc_t &rl = p->get_loc_stack ()->back ();
    str fn = rl.filename ();
    int l = rl.lineno ();
    strbuf b ("%s:%d", fn ? fn.cstr () : "<unknown>" , l);
    return expr_str_t::alloc (b);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  stacktrace_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
      pub3::obj_list_t stack;
      for (const runloc_t& frame: *p->get_loc_stack()) {
          pub3::obj_t o;
          o("filename") = frame.filename();
          o("line") = frame.lineno();
          o("function") = frame.funcname();
          stack.push_back(o);
      }
    return stack.obj();
  }

  //-----------------------------------------------------------------------

  const str match_t::DOCUMENTATION =
    "Given a regex //r// and a string //s//, return //true// "
    "if //r// matches the **whole** string //s// and //false// otherwise.";

  //-----------------------------------------------------------------------

  const str search_t::DOCUMENTATION =
    "Given a regex //r// and a string //s//, split //s// into "
    "a list of strings, akin to the Python function 'split'.  Return a list "
    "of strings.";

  //-----------------------------------------------------------------------

  const str replace_t::DOCUMENTATION =
    "In the string //body//, find all occurrence of "
    "the regular expression //pat// and replace with the string //repl//, "
    "or the output of the lambda //repl//.  By default, a string //repl// "
    "can contain Perl-like capture groups (e.g., '<b>$0</b>'), but if you "
    "don't want that, pass a fourth optional parameter //use_captures// "
    "as //false//.";

};
