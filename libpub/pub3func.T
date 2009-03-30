// -*-c++-*-
#include "pub3func.h"

//-----------------------------------------------------------------------

bool
pub3::for_t::add (ptr<expr_list_t> l)
{
  bool ret = true;
  if (!l || l->size () < 1 || l->size () > 2) {
    PWARN ("for: takes 2 arguments (simple identifier and array)\n");
    ret = false;
  } else if (!(_iter = (*l)[0]->eval_as_identifier ()) || !_iter.len ()) {
    PWARN ("for: argument 1 must be an identifier\n");
    ret = false;
  } else if (l->size () > 1) {
    _arr = (*l)[1];
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::include_t::add (ptr<expr_list_t> l)
{
  bool ret = true;
  if (!l || l->size () < 1 || l->size () > 2) {
    PWARN ("include takes 1 or 2 arguments (file and dictionary/bindings)");
    ret = false;
  } else {
    _file = (*l)[0];
    if (l->size () > 1) { _dict = (*l)[1]; }
  }
  return ret;
}

//-----------------------------------------------------------------------

void
pub3::for_t::output (output_t *o, penv_t *e) const
{
}

//-----------------------------------------------------------------------

void
pub3::include_t::output (output_t *o, penv_t *e) const
{
}

//-----------------------------------------------------------------------

tamed void
pub3::include_t::publish (pub2_iface_t *iface, output_t *o, penv_t *e,
			  xpub_status_cb_t cb) const
{
  tvars {
    xpub_status_t res (XPUB_STATUS_OK);
    pub3::eval_t eval (e, o);
    ptr<const aarr_t> d;
    str fn;
    aarr_t *ccd;
  }

  if (!(fn = _file->eval_as_str (eval)) || fn.len () == 0) {
    strbuf b ("include: cannot evaluate filename");
    o->output_err (e, b, lineno);
  } else if (_dict && !(d = _dict->eval_as_dict (eval))) {
    strbuf b ("include: cannot evaluate dictionary argument");
    o->output_err (e, b, lineno);
  } else {
   
    // unfortunately, don't see a good way around this just now..
    ccd = const_cast<aarr_t *> (static_cast<const aarr_t *> (d));

    twait { pfile_func_t::publish (iface, o, e, ccd, fn, mkevent (res)); }
  }

  (*cb) (res);
}

//-----------------------------------------------------------------------

tamed void
pub3::for_t::publish (pub2_iface_t *iface, output_t *o, penv_t *e,
		      xpub_status_cb_t callercv) const
{
  tvars {
    ptr<const parr_mixed_t> v;
    bool ok (false);
    aarr_t frame;
    size_t i, len;
    ptr<pval_t> row;
    ptr<aarr_arg_t> row_aarr;
    xpub_status_t res (XPUB_STATUS_OK);
    size_t frame_pointer;
    pub3::eval_t eval (e, o);
  }

  { 
    bool old_silent = eval.set_silent (true);
    v = _arr->eval_as_vec (eval);
    eval.set_silent (old_silent);
  }

  if (!v && !_empty) {

    strbuf b ("for: second argument is not an iterable vector");
    o->output_err (e, b, lineno);
    
  } else if (!v) {

    twait { _empty->publish (iface, o, e, mkevent (res)); }

  } else {

    ok = true;
    len = v->size ();

    frame_pointer = e->size ();
    e->push (&frame);

    for (i = 0; i < len; i++) {
      if (!(row = (*v)[i])) {
	strbuf b ("for: row %zu is undefined!", i);
	o->output_err (e, b, lineno);
      } else if (!(row_aarr = row->to_aarr ())) {
	strbuf b ("for: row %zu is not an associative array!", i);
	o->output_err (e, b, lineno);
      } else if (_env) {
	row_aarr->replace ("count", int64_t (len));
	row_aarr->replace ("iter",  int64_t (i));
	row_aarr->replace ("last",  int64_t (len - 1 == i));
	row_aarr->replace ("first", int64_t (i == 0));
	row_aarr->replace ("odd",   int64_t (i % 2));
	row_aarr->replace ("even",  1 - int64_t (i % 2));
	frame.replace (_iter, row_aarr);

	e->bump ();
	twait { _env->publish (iface, o, e, mkevent (res)); }
      }
    }
    e->resize (frame_pointer);
  }
  
  (*callercv) (res);
}

//-----------------------------------------------------------------------

bool
pub3::include_t::publish_nonblock (pub2_iface_t *i, output_t *o, 
				   penv_t *e) const
{
  return false;
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
	expr->eval_as_bool (eval)) {

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
pub3::set_func_t::output_runtime (output_t *o, penv_t *e) const
{
  ptr<aarr_t> aarr_global;

  eval_t eval (e, o);
  if ((aarr_global = e->get_global_aarr ())) {
    e->bump ();
    eval.flatten_dict (aarr, aarr_global);
  } else {
    push_frame (e, aarr);
  }
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
  error_fn_t::eval_as_pval (eval_t e) const
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
