// -*-c++-*-

#include "okrfnlib.h"
#include "okformat.h"
#include "pub3parse.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  ptr<expr_t> 
  dict_fn_t::eval_pval (eval_t e, const pval_t *in) const
  {
    ptr<const expr_t> x;
    ptr<pval_t> v2;
    ptr<expr_t> x2;

    if (!in ||
	!(x = in->to_expr ()) || 
	!(v2 = x->eval_freeze (e)) ||
	!(x2 = v2->to_expr ())) {

      x2 = expr_null_t::alloc ();
    }

    return x2;
  }

  //-----------------------------------------------------------------------

  const nvtab_t *
  dict_fn_t::eval_to_nvtab (eval_t e, const char *fn) const
  {
    const nvtab_t *nvt =  NULL;
    ptr<const aarr_t> aa;
    if ((aa = _arg->eval_as_dict (e))) {
      nvt = aa->nvtab ();
    } else {
      report_error (e, strbuf ("%s() take a dictionary argument", fn));
    }
    return nvt;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  values_t::eval_internal (eval_t e) const
  {
    const nvtab_t *nvt = eval_to_nvtab (e, "values");
    ptr<pval_t> ret;
    if (nvt) {
      ptr<expr_list_t> l = New refcounted<expr_list_t> ();
      for (nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
	l->push_back (eval_pval (e, p->value ()));
      }
      ret = l;
    } else {
      ret = expr_null_t::alloc ();
    }

    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  keys_t::eval_internal (eval_t e) const
  {
    ptr<pval_t> ret;
    const nvtab_t *nvt = eval_to_nvtab (e, "keys");
    if (nvt) {
      ptr<expr_list_t> l = New refcounted<expr_list_t> ();
      for (nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
	l->push_back (New refcounted<expr_str_t> (p->name ()));
      }
      ret = l;
    } else {
      ret = expr_null_t::alloc ();
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  items_t::eval_internal (eval_t e) const
  {
    ptr<pval_t> ret;
    const nvtab_t *nvt = eval_to_nvtab (e, "itesm");
    if (nvt) {
      ptr<expr_list_t> l = New refcounted<expr_list_t> ();
      for (nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
	ptr<expr_list_t> entry = New refcounted<expr_list_t> ();
	entry->push_back (New refcounted<expr_str_t> (p->name ()));
	entry->push_back (eval_pval (e, p->value()));
	l->push_back (entry);
      }
      ret = l;
    } else {
      ret = expr_null_t::alloc ();
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  json2pub_t::json2pub_t (const str &n, ptr<expr_list_t> el, int l, 
			  ptr<expr_t> x)
    : runtime_fn_t (n, el, l), _json (x) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  json2pub_t::constructor (const str &n, ptr<expr_list_t> e, int lineno,
			   str *err)
  {
    ptr<runtime_fn_t> ret;
    size_t narg = e ? e->size () : size_t (0);

    if (narg != 1) {
      *err = "json2pub() takes one argument (a string)";
    } else {
      ret = New refcounted<json2pub_t> (n, e, lineno, (*e)[0]);
    }
    return ret;
  }

  //-----------------------------------------------------------------------
  
  ptr<expr_t> 
  json2pub_t::eval_internal (eval_t e) const
  {
    ptr<expr_t> ret;
    str s = _json->eval_as_str (e);
    if (s) { ret = json_parser_t::parse (s); }
    if (!ret) ret = expr_null_t::alloc ();
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t> json2pub_t::eval_freeze (eval_t e) const 
  { return eval_internal (e); }

  //-----------------------------------------------------------------------

  ptr<const pval_t> json2pub_t::eval (eval_t e) const 
  { return eval_internal (e); }

  //-----------------------------------------------------------------------

};
