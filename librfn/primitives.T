// -*-c++-*-

#include "okrfn.h"
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

  ptr<const expr_t>
  version_str_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    return expr_str_t::alloc (version_str ());
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  version_int_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  { 
    return expr_uint_t::alloc (version_int ());
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  okws_version_str_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  { return expr_str_t::alloc (okws_version_str ()); }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  okws_version_int_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  { return expr_uint_t::alloc (okws_version_int ()); }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  dump_env_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> l = p->env ()->to_list ();
    return l;
  }

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

  ptr<const expr_t>
  str_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const pub3::expr_t> e = args[0]._O;
    str s;
    if (e) s = e->to_str (false);
    return expr_str_t::safe_alloc (s);
  }

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

  ptr<const expr_t>
  enable_wss_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    bool ret = p->out ()->enable_wss (args[0]._b);
    return expr_bool_t::alloc (ret);
  }

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

  ptr<const expr_t>
  copy_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    return args[0]._O->copy ();
  }

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

};
