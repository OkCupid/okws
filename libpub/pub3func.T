// -*-c++-*-
#include "pub3func.h"

//-----------------------------------------------------------------------

bool
pub3::for_t::add (ptr<arglist_t> l)
{
  bool ret = true;
  if (_iter || 
      !l || 
      l->size () != 2 ||
      !(_iter = ((*l)[0])->eval ()) ||
      !(_arr = ((*l)[1])->eval ())) {
    PWARN ("pub takes 2 arguments (formal variable and array)\n");
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
pub3::for_t::output (output_t *o, penv_t *e) const
{
}

//-----------------------------------------------------------------------

tamed void
pub3::for_t::publish (pub2_iface_t *iface, output_t *o, penv_t *e,
		      xpub_status_cb_t callercv) const
{
  tvars {
    const parr_mixed_t *a (NULL);
    const pval_t *v (NULL);
    ptr<const pval_t> hold;
    bool ok (false);
    aarr_t frame;
    size_t i, len;
    ptr<pval_t> row;
    ptr<aarr_arg_t> row_aarr;
    xpub_status_t res (XPUB_STATUS_OK);
    size_t frame_pointer;
  }

  v = e->lookup (_arr, false);
  if (v) a = v->to_mixed_arr ();

  if (!v && !_empty) {

    strbuf b ("for: cannot find iteration value (%s)", _arr.cstr ());
    o->output_err (e, b, lineno);

  } else if (!a && !_empty) {

    strbuf b ("for: given value is not iterable vector (%s)", _arr.cstr ());
    o->output_err (e, b, lineno);
    
  } else if (!a) {

    twait { _empty->publish (iface, o, e, mkevent (res)); }

  } else {

    hold = mkref (v);

    ok = true;
    assert (a);
    len = a->size ();

    frame_pointer = e->size ();
    e->push (&frame);

    for (i = 0; i < len; i++) {
      if (!(row = (*a)[i])) {
	strbuf b ("for: row %zu is undefined!", i);
	o->output_err (e, b, lineno);
      } else if (!(row_aarr = row->to_aarr ())) {
	strbuf b ("for: row %zu is not an associative array!", i);
	o->output_err (e, b, lineno);
      } else if (_env) {
	row_aarr->replace ("count", int64_t (len));
	row_aarr->replace ("iter", int64_t (i));
	row_aarr->replace ("last", int64_t (len - 1 == i));
	row_aarr->replace ("odd", int64_t (i % 2));
	row_aarr->replace ("even", 1 - int64_t (i % 2));
	frame.replace (_iter, row_aarr);

	twait { _env->publish (iface, o, e, mkevent (res)); }
      }
    }
    e->resize (frame_pointer);
  }
  
  (*callercv) (res);
}

//-----------------------------------------------------------------------

bool
pub3::for_t::publish_nonblock (pub2_iface_t *i, output_t *o, penv_t *e) const
{
  return false;
}

//-----------------------------------------------------------------------

bool
pub3::cond_t::publish_nonblock (pub2_iface_t *i, output_t *o, penv_t *e) const
{
  return false;
}

//-----------------------------------------------------------------------

tamed void
pub3::cond_t::publish (pub2_iface_t *iface, output_t *o, penv_t *e,
		      xpub_status_cb_t callercv) const
{
  tvars {
    xpub_status_t res (XPUB_STATUS_OK);
    size_t len;
    size_t i;
    ptr<const pub3::expr_t> expr;
    bool found (false);
    pub3::eval_t eval (e, o);
    ptr<pub3::cond_clause_t> clause;
    ptr<nested_env_t> env;
  }

  len = _clauses ? _clauses->size () : size_t (0) ;

  for (i = 0; !found && i < len; i++) {

    if ((clause = (*_clauses)[i]) &&
	(expr = clause->expr ()) &&
	expr->eval_as_bool (&eval)) {

      found = true;
      if ((env = clause->env ())) {
	twait { env->publish (iface, o, e, mkevent (res)); }
      }
    }
  }

  (*callercv) (res);
}

//-----------------------------------------------------------------------

void
pub3::cond_t::output (output_t *o, penv_t *e) const
{
}

//-----------------------------------------------------------------------
//
// Functions for allocations runtime functions (rfn's)

namespace pub3 {

  //-----------------------------------------------------------------------

  ptr<rfn_factory_t> rfn_factory_t::_curr;

  //-----------------------------------------------------------------------
  
  void
  rfn_factory_t::set (ptr<rfn_factory_t> f)
  {
    _curr = f;
  }
  
  //-----------------------------------------------------------------------
  
  ptr<rfn_factory_t>
  rfn_factory_t::get ()
  {
    if (!_curr) {
      _curr = New refcounted<null_rfn_factory_t> ();
    }
    return _curr;
  }
    
  //-----------------------------------------------------------------------

  ptr<const pval_t>
  error_fn_t::eval_as_pval (eval_t *e) const
  {
    report_error (e, _err);
    return NULL;
  }

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t> 
  null_rfn_factory_t::alloc (const str &s, ptr<expr_list_t> l, int lineno)
  {
    strbuf b ("null library; function '%s' does not exist here", s.cstr ());
    return New refcounted<error_fn_t> (s, l, lineno, b);
  }

  //-----------------------------------------------------------------------

};

//
// end RFNs
//-----------------------------------------------------------------------
