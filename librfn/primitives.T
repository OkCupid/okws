// -*-c++-*-

#include "okrfn.h"
#include "okformat.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  tamed void
  isnull_t::pub_to_val (publish_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      bool b;
    }
    twait { (*args)[0]->pub_as_null (p, mkevent (b)); }
    ret = expr_bool_t::alloc (b);
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  len_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
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
  type_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
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
  version_str_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    return expr_str_t::alloc (version_str ());
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  version_int_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  { 
    return expr_uint_t::alloc (version_int ());
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  okws_version_str_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  { return expr_str_t::alloc (okws_version_str ()); }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  okws_version_int_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  { return expr_uint_t::alloc (okws_version_int ()); }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  dump_env_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<expr_list_t> l = p->env ()->to_list ();
    return l;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  int_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<const pub3::expr_t> e = args[0]._O;
    int64_t i = 0;
    if (e) i = e->to_int ();
    return expr_int_t::alloc (i);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  str_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<const pub3::expr_t> e = args[0]._O;
    str s;
    if (e) s = e->to_str (false);
    return expr_str_t::safe_alloc (s);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  warn_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_t> ret = expr_null_t::alloc ();
    str s = args[0]._s;
    if (!s) s = "";
    report_error (p, s);
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  warn_trace_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_t> ret = expr_null_t::alloc ();
    str s = args[0]._s;
    if (!s) s = "";
    p->output_err_stacktrace (s, P_ERR_EVAL);
    return ret;
  }

  //-----------------------------------------------------------------------

};
