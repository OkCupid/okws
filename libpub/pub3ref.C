#include "pub3expr.h"
#include "pub3parse.h"

namespace pub3 {

  //=============================== expr_ref_t ============================

  ptr<expr_t>
  expr_ref_t::eval_to_mval (eval_t *e) const
  {
    ptr<expr_t> ret;
    ptr<mref_t> r = eval_to_ref (e);
    if (r) ret = r->get_value ();
    return ret;
  }

  //=============================== expr_dictref_t ========================

  ptr<expr_dictref_t>
  expr_dictref_t::alloc (ptr<expr_t> d, const str &n)
  {
    return New refcounted<expr_dictref_t> (d, n, plineno ());
  }

  //--------------------------------------------------------------------

  ptr<const expr_t> 
  expr_dictref_t::eval_to_val (eval_t *e) const
  {
    ptr<const expr_t> d = _dict->eval_to_val (e);
    return eval_to_val_final (e, d);
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_dictref_t::eval_to_val_final (eval_t *e, ptr<const expr_t> x) const
  {
    ptr<const expr_dict_t> d;
    const ptr<expr_t> *valp;
    ptr<expr_t> out;
    if (!x) {
      report_error (e, "failed to evaluate expression (as a dictionary)");
    } else if (!(d = x->to_dict ())) {
      report_error (e, "can't coerce value to dictionary");
    } else if ((valp = (*d)[_key])) {
      out = *valp;
    } else {
      //out = expr_null_t::alloc ();
    }
    return out;
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_dictref_t::eval_to_ref (eval_t *e) const
  {
    ptr<mref_t> dr = _dict->eval_to_ref (e);
    return eval_to_ref_final (e, dr);
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_dictref_t::eval_to_ref_final (eval_t *e, ptr<mref_t> dr) const
  {
    ptr<expr_t> x;
    ptr<expr_dict_t> d;
    ptr<mref_t> r;

    if (!dr) {
      report_error (e, "failed to evaluate dictionary");
    } else if (!(x = dr->get_value ())) {
      report_error (e, "the dictionary referred to was null");
    } else if (!(d = x->to_dict ())) {
      report_error (e, "can't coerce value to dictionary");
    } else {
      r = New refcounted<mref_dict_t> (d, _key);
    }
    return r;
  }

  //--------------------------------------------------------------------

  bool expr_dictref_t::might_block_uncached () const
  { return expr_t::might_block (_dict); }

  //====================================================================

  ptr<expr_varref_t> expr_varref_t::alloc (const str &n)
  { return New refcounted<expr_varref_t> (n, plineno ()); }

  //--------------------------------------------------------------------

  void
  expr_varref_t::report (eval_t *e, bool out) const
  {
    if (!out && !e->silent() && (e->opts () & P_WARN_NULL)) {
      strbuf b ("cannot resolve variable: '%s'", _name.cstr ());
      report_error (e, b);
    }
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_varref_t::eval_to_val (eval_t *e) const
  {
    ptr<const expr_t> ret = e->lookup_val (_name);
    report (e, ret);
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_varref_t::eval_to_ref (eval_t *e) const
  {
    ptr<mref_t> ret = e->lookup_ref (_name);
    report (e, ret);
    return ret;
  }

  //--------------------------------------------------------------------

  void
  expr_varref_t::pub_to_ref (eval_t *p, mrev_t ev, ptr<closure_t> d) const
  { ev->trigger (eval_to_ref (p)); }

  //--------------------------------------------------------------------

  void
  expr_varref_t::pub_to_val (eval_t *p, cxev_t ev, ptr<closure_t> d) const
  { ev->trigger (eval_to_val (p)); }

  //--------------------------------------------------------------------

  void
  expr_varref_t::v_dump (dumper_t *d) const
  {
    d->dump (_name, true);
  }

  //===================================================================

  namespace {
    typedef expr_scoped_varref_t::scope_t vscope_t;
    const char * vscope_to_string(const vscope_t v) {
      switch (v) {
      case vscope_t::GLOBALS:    return "globals";
      case vscope_t::UNIVERSALS: return "universals";
      }
    }

    ptr<bindtab_t> get_layer(const vscope_t v, eval_t *e) {
      switch (v) {
      case vscope_t::GLOBALS:    return e->env()->globals();
      case vscope_t::UNIVERSALS: return e->env()->universals();
      }
    }
  }  // namespace

  ptr<expr_scoped_varref_t> expr_scoped_varref_t::alloc (const str &n,
                                                         const scope_t sc)
  { return New refcounted<expr_scoped_varref_t> (n, sc, plineno ()); }

  //--------------------------------------------------------------------

  void
  expr_scoped_varref_t::report (eval_t *e, bool out) const
  {
    if (!out && !e->silent() && (e->opts () & P_WARN_NULL)) {
      strbuf b ("cannot resolve variable: '%s::%s'",
                vscope_to_string(_scope),
                _name.cstr ());
      report_error (e, b);
    }
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_scoped_varref_t::eval_to_val (eval_t *e) const
  {
    ptr<const expr_t> ret;
    ptr<const bindtab_t> bt = get_layer(_scope, e);
    bt->lookup(_name, &ret);
    if (!ret) { ret = expr_null_t::alloc (); }
    report (e, ret);
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_scoped_varref_t::eval_to_ref (eval_t *e) const
  {
    ptr<bindtab_t> bt = get_layer(_scope, e);
    ptr<bindtab_t> layer = bt->mutate();
    ptr<mref_t> ret = New refcounted<mref_dict_t>(layer, _name);;
    report (e, ret);
    return ret;
  }

  //--------------------------------------------------------------------

  void
  expr_scoped_varref_t::pub_to_ref (eval_t *p, mrev_t ev, ptr<closure_t> d) const
  { ev->trigger (eval_to_ref (p)); }

  //--------------------------------------------------------------------

  void
  expr_scoped_varref_t::pub_to_val (eval_t *p, cxev_t ev, ptr<closure_t> d) const
  { ev->trigger (eval_to_val (p)); }

  //--------------------------------------------------------------------

  void
  expr_scoped_varref_t::v_dump (dumper_t *d) const
  {
    d->dump (strbuf ("%s::%s", vscope_to_string(_scope), _name.cstr()), false);
  }

  //====================================================================

  ptr<expr_vecref_t>
  expr_vecref_t::alloc (ptr<expr_t> v, ptr<expr_t> i)
  {
    return New refcounted<expr_vecref_t> (v, i, plineno ());
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_vecref_t::eval_to_ref (eval_t *e) const
  {
    ptr<mref_t> cr = _vec->eval_to_ref (e);
    ptr<const expr_t> i = _index->eval_to_val (e);
    return eval_to_ref_final (e, cr, i);
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_vecref_t::eval_to_ref_final (eval_t *e, ptr<mref_t> cr, 
				    ptr<const expr_t> i) const
  {
    ptr<expr_t> c;
    ptr<expr_dict_t> d;
    ptr<expr_list_t> l;
    ptr<mref_t> ret;

    if (!cr || !(c = cr->get_value ())) {
      report_error (e, "container evaluates to null");
    } else if (!i) {
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
  expr_vecref_t::eval_to_val (eval_t *e) const
  {
    ptr<const expr_t> c = _vec->eval_to_val (e);
    ptr<const expr_t> i = _index->eval_to_val (e);
    return eval_to_val_final (e, c, i);
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_vecref_t::eval_to_val_final (eval_t *e, ptr<const expr_t> c, 
				    ptr<const expr_t> i) const
  {
    ptr<const expr_dict_t> d;
    ptr<const expr_list_t> l;
    ptr<const expr_t> ret;

    if (!c || c->is_null ()) {
      report_error (e, "container evaluates to null");
    } else if (!i || i->is_null ()) {
      report_error (e, "cannot evaluate key for lookup");
    } else if ((d = c->to_dict ())) {
      str k;
      if ((k = i->to_str ())) {
	ret = d->lookup (k);
      } else {
	report_error (e, "cannot coerce dictionary index to string");
      }
    } else if ((l = c->to_list ())) {
      int64_t ii;
      if (i->to_int (&ii)) {
	ret = l->lookup (ii);
      } else {
	report_error (e, "indices into lists must be integers");
      }
    } else {
      report_error (e, "[]-reference into an object not a dict or list");
    }
    return ret;
  }
  //--------------------------------------------------------------------

  bool expr_vecref_t::might_block_uncached () const
  { return expr_t::might_block (_vec, _index); }

  //================================= const_mref_t =====================

  void
  const_mref_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "expr", _x);
  }

  //========================== mref_dict_t =============================

  void
  mref_dict_t::v_dump (dumper_t *d) const
  {
    d->dump (strbuf ("slot: %s", _slot.cstr ()), true);
    // XXX print dict at some point.
  }
  
  //========================== mref_list_t =============================

  void
  mref_list_t::v_dump (dumper_t *d) const
  {
    d->dump (strbuf ("index: %zd", _index), true);
    s_dump (d, "list", _list);
  }

  //===================================================================
  
  

};
