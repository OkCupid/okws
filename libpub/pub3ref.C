#include "pub3expr.h"

namespace pub3 {

  //====================================================================

  ptr<const expr_t> 
  expr_dictref_t::eval_to_val (eval_t e) const
  {
    ptr<const expr_t> x;
    ptr<const expr_dict_t> d;
    ptr<const expr_t> *valp;
    ptr<expr_t> out;
    assert (_dict);
    if (!(x = _dict->eval_to_val (e))) {
      report_error (e, "failed to evaluate expression (as a dictionary)");
    } else if (!(d = x->to_dict ())) {
      report_error (e, "can't coerce value to dictionary");
    } else if ((valp = (*d)[_key])) {
      out = *valp;
    } else {
      out = expr_null_t::alloc ();
    }
    return out;
  }

  //--------------------------------------------------------------------

  ptr<expr_t>
  expr_dictref_t::eval_to_rhs (eval_t e) const
  {
    ptr<expr_t> x;
    ptr<expr_dict_t> d;
    ptr<expr_t> *valp;
    assert (_dict);
    if (!(x = _dict->eval_to_rhs (e))) {
      report_error (e, "failed to evaluate expression (as a dictionary)");
    } else if (!(d = x->to_dict ())) {
      report_error (e, "can't coerce value to dictionary");
    } else if ((valp = (*d)[_key])) {
      out = *valp;
    } else {
      out = expr_null_t::alloc ();
    }
    return out;
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_dictref_t::eval_to_lhs (eval_t e) const
  {
    ptr<expr_t> x;
    ptr<expr_dict_t> d;
    ptr<mref_t> r;
    assert (_dict);
    if (!(x = _dict->eval_to_rhs (e))) {
      report_error (e, "failed to evaluate expression (as a dictionary)");
    } else if (!(d = x->to_dict ())) {
      report_error (e, "can't coerce value to dictionary");
    } else {
      r = New refcounted<mref_dict_t> (d, _key);
    }
    return r;
  }

  //====================================================================

  ptr<const expr_t>
  expr_varref_t::eval_to_val (eval_t e) const
  {
    return e.lookup_val (_name);
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_varref_t::eval_to_lhs (eval_t e) const
  {
    return e.lookup_ref (e);
  }

  //====================================================================

  ptr<expr_t>
  expr_vecref_t::eval_to_rhs (eval_t e) const
  {
    ptr<expr_dict_t> d;
    ptr<expr_list_t> l;
    ptr<expr_t> out;
    int64_t i;
    str k;

    if (!eval_rhs_prepare (&d, &l, &k, &i)) {
      /* noop */
    } else if (d && k) {
      out = d->lookup (k);
    } else if (l) {
      out = d->lookup (i);
    }
    return out;
  }

  //--------------------------------------------------------------------

  bool
  expr_vecref_t::eval_rhs_prepare (ptr<expr_dict_t> *dp, ptr<expr_list_t> *lp,
				   str *kp, int64_t *ip)
  {
    ptr<expr_t> c;
    ptr<const expr_t> i;
    bool ret = false;

    assert (_vec);
    assert (_index);

    if (!(c = _vec->eval_to_rhs (e))) {
      report_error (e, "container evaluates to null");
    } else if (!(i = _index->eval_to_val (e))) {
      report_error (e, "cannot evaluate key for lookup");
    } else if ((*dp = c->to_dict ())) {
      if ((*kp = i->to_str ())) {
	ret = true;
      } else {
	report_error (e, "cannot coerce dictionary index to string");
      }
    } else if ((*lp = c->to_list ())) {
      if (i->to_int (ip)) {
	ret = true;
      } else {
	report_error (e, "indices into lists must be integers");
      }
    } else {
      report_error (e, "[]-reference into an object not a dict or list");
    }
    return ret;
  }

  //--------------------------------------------------------------------

  bool
  expr_vecref_t::eval_val_prepare (ptr<const expr_dict_t> *dp, 
				   ptr<const expr_list_t> *lp,
				   str *kp, int64_t *ip)
  {
    ptr<const expr_t> c;
    ptr<const expr_t> i;
    bool ret = false;

    assert (_vec);
    assert (_index);

    if (!(c = _vec->eval_to_val (e))) {
      report_error (e, "container evaluates to null");
    } else if (!(i = _index->eval_to_val (e))) {
      report_error (e, "cannot evaluate key for lookup");
    } else if ((*dp = c->to_dict ())) {
      if ((*kp = i->to_str ())) {
	ret = true;
      } else {
	report_error (e, "cannot coerce dictionary index to string");
      }
    } else if ((*lp = c->to_list ())) {
      if (i->to_int (ip)) {
	ret = true;
      } else {
	report_error (e, "indices into lists must be integers");
      }
    } else {
      report_error (e, "[]-reference into an object not a dict or list");
    }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_vecref_t::eval_to_val (eval_t e) const
  {
    ptr<const expr_dict_t> d;
    ptr<const expr_list_t> l;
    str k;
    int64_t i;
    ptr<const expr_t> out;

    if (!eval_val_prepare (&d, &l, &k, &i)) {
      /* noop */
    } else if (d && k) {
      out = d->lookup (k);
    } else if (l) {
      out = l->lookup (i);
    }

    return out;
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_vecref_t::eval_to_lhs (eval_t e) const
  {
    ptr<expr_dict_t> d;
    ptr<expr_list_t> l;
    str k;
    int64_t i;
    ptr<mref_t> out;

    if (!eval_rhs_prepare (&d, &l, &k, &i)) {
      /* noop */
    } else if (d && k) {
      out = New refcounted<mref_dict_t> (d, k);
    } else if (l) {
      out = New refcounted<mref_list_t> (l, ii);
    }
    return out;
  }

  //====================================================================
  
};
