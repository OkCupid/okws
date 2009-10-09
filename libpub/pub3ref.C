#include "pub3expr.h"

namespace pub3 {

  //====================================================================

  ptr<expr_dictref_t>
  expr_dictref_t::alloc (ptr<expr_t> d, const str &n)
  {
    return New refcounted<expr_dictref_t> (d, n, plineno ());
  }

  //--------------------------------------------------------------------

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

  ptr<mref_t>
  expr_dictref_t::eval_to_ref (eval_t e) const
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

  ptr<expr_varref_t> expr_varref_t::alloc (const str &n)
  { return New refcounted<expr_varref_t> (n, plineno ()); }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_varref_t::eval_to_val (eval_t e) const
  {
    return e.lookup_val (_name);
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_varref_t::eval_to_ref (eval_t e) const
  {
    return e.lookup_ref (e);
  }

  //====================================================================

  ptr<expr_vecref_t>
  expr_vecref_t::alloc (ptr<expr_t> v, ptr<expr_t> i)
  {
    return New refcounted<expr_vecref_t> (v, i, plineno ());
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_vecref_t::eval_to_ref (eval_t e) const
  {
    ptr<expr_t> c;
    ptr<const expr_t> i;
    ptr<expr_dict_t> d;
    ptr<expr_list_t> l;
    ptr<mref_t> ret;

    assert (_vec);
    assert (_index);

    if (!(c = _vec->eval_to_rhs (e))) {
      report_error (e, "container evaluates to null");
    } else if (!(i = _index->eval_to_val (e))) {
      report_error (e, "cannot evaluate key for lookup");
    } else if ((d = c->to_dict ())) {
      str k;
      if ((k = i->to_str ())) {
	ret = mref_dict_t::alloc (d, k);
      } else {
	report_error (e, "cannot coerce dictionary index to string");
      }
    } else if ((l = c->to_list ())) {
      int64_t ii;
      if (i->to_int (&ii)) {
	ret = mref_list_t::alloc (l, ii);
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
    ptr<const expr_t> c;
    ptr<const expr_t> i;
    ptr<const expr_dict_t> d;
    ptr<const expr_list_t> l;
    ptr<const expr_t> ret;

    assert (_vec);
    assert (_index);

    if (!(c = _vec->eval_to_val (e))) {
      report_error (e, "container evaluates to null");
    } else if (!(i = _index->eval_to_val (e))) {
      report_error (e, "cannot evaluate key for lookup");
    } else if ((d = c->to_dict ())) {
      str k;
      if ((k = i->to_str ())) {
	ret = mref_dict_t::alloc (d, k);
      } else {
	report_error (e, "cannot coerce dictionary index to string");
      }
    } else if ((l = c->to_list ())) {
      int64_t ii;
      if (i->to_int (ip)) {
	ret = mref_list_t::alloc (l, ii);
      } else {
	report_error (e, "indices into lists must be integers");
      }
    } else {
      report_error (e, "[]-reference into an object not a dict or list");
    }
    return ret;
  }
  //====================================================================
  
};
