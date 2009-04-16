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
  } else if (!(_iter = (*l)[0]->to_identifier ()) || !_iter.len ()) {
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
    ptr<const vec_iface_t> v;
    bool ok (false);
    aarr_t frame;
    size_t i, len;
    ptr<const pval_t> crow;
    ptr<const pub3::expr_t> cex;
    ptr<const pub3::expr_dict_t> row_dict;
    xpub_status_t res (XPUB_STATUS_OK);
    size_t frame_pointer;
    pub3::eval_t eval (e, o);
    ptr<aarr_arg_t> row_aarr;
    ptr<pval_t> row_val;
  }

  { 
    bool old_silent = eval.set_silent (true);
    v = _arr->eval_as_vec (eval);
    eval.set_silent (old_silent);
  }

  if (!v && !_empty) {

    strbuf b ("for: second argument is not an iterable vector");
    o->output_err (e, b, lineno);
    
  } else if (!v || v->size () == 0) {

    twait { _empty->publish (iface, o, e, mkevent (res)); }

  } else {

    ok = true;
    len = v->size ();

    for (i = 0; i < len; i++) {

      row_val = NULL;

      // XXX would like to not have this const-cast, but I don't see
      // a way around it other than redoing the whole eval_as_pval
      // mechanism, or to copy the aarr, which is slow.  Revisit.
      if (!(crow = v->lookup (i))) {
	strbuf b ("for: row %zu is undefined!", i);
	o->output_err (e, b, lineno);
      } else if (!_env) {
	/* noop */
      } else if ((row_dict = crow->to_expr_dict ())) {
	row_aarr = row_dict->copy_dict_stub ();
	row_aarr->replace ("count", expr_int_t::alloc (int64_t (len)));
	row_aarr->replace ("iter",  expr_int_t::alloc (int64_t (i)));
	row_aarr->replace ("last",  expr_int_t::alloc (int64_t (len - 1 == i)));
	row_aarr->replace ("first", expr_int_t::alloc (int64_t (i == 0)));
	row_aarr->replace ("odd",   expr_int_t::alloc (int64_t (i % 2)));
	row_aarr->replace ("even",  expr_int_t::alloc (1 - int64_t (i % 2)));
	row_val = row_aarr;
      } else if ((cex = crow->to_expr ())) {
	row_val = cex->copy_stub ();
      } else {
	strbuf b ("for: row %zu cannot be assigned", i);
	o->output_err (e, b, lineno);
      }

      if (row_val) {

	// reuse the aarr 'frame' to save on allocations, etc.
	frame.replace (_iter, row_val);

	// active the frame with the new iteration variables.
	frame_pointer = e->push (&frame);

	// refresh the version number to cleared cached evaluations
	e->bump ();

	twait { _env->publish (iface, o, e, mkevent (res)); }

	// restore eval stack once per iteration.  this will also
	// undo any setl's, import's, etc, from within the body of
	// this loop.
	e->resize (frame_pointer);
      }
    }
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

    // Note that expr will be NULL for the last (else) clause if
    // one was there....
    if ((clause = (*_clauses)[i]) &&
	(!(expr = clause->expr ()) ||
	 (expr && expr->eval_as_bool (eval)))) {

      found = true;
      if ((env = clause->env ())) {
	twait { env->publish (iface, o, e, mkevent (res)); }
      }
    }
  }

  (*callercv) (res);
}

//-----------------------------------------------------------------------

bool
pub3::print_t::add (ptr<pub3::expr_list_t> l)
{
  bool ret = true;
  if (!_args) {
    _args = l;
  } else {
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::print_t::publish_nonblock (pub2_iface_t *f, output_t *o,
				 penv_t *e) const
{
  pub3::eval_t evl (e, o);
  str s;
  for (size_t i = 0; _args && i < _args->size (); i++) {
    ptr<const expr_t> x = (*_args)[i];
    if (!x) { 
      /* noop */
    } else if (_silent) {
      (void) x->eval (evl);
    } else if ((s = x->eval_as_str (evl))) {
      o->output (e, s, false);
    }
  }
  return true;
}

//-----------------------------------------------------------------------

void
pub3::print_t::output (output_t *o, penv_t *e) const
{
}

//-----------------------------------------------------------------------

void
pub3::set_func_t::output_runtime (output_t *o, penv_t *e) const
{
  ptr<aarr_t> aarr_global;

  eval_t eval (e, o);
  if ((aarr_global = e->get_global_aarr ())) {
    e->bump ();
    eval.eval_freeze_dict (aarr, aarr_global);
  } else {
    e->push (aarr);
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
  error_fn_t::eval (eval_t e) const
  {
    report_error (e, _err);
    return NULL;
  }

  //-----------------------------------------------------------------------

  ptr<pval_t>
  error_fn_t::eval_freeze (eval_t e) const
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

  ptr<const expr_t>
  runtime_fn_stub_t::get_rfn () const
  {
    if (!_rfn) {
      _rfn = rfn_factory_t::get ()->alloc (_name, _arglist, _lineno);
    }
    return _rfn;
  }

  //-----------------------------------------------------------------------

  ptr<const pval_t> 
  runtime_fn_stub_t::eval (eval_t e) const
  {
    return get_rfn ()->eval (e);
  }

  //-----------------------------------------------------------------------

  ptr<pval_t> 
  runtime_fn_stub_t::eval_freeze (eval_t e) const
  {
    return get_rfn ()->eval_freeze (e);
  }

  //-----------------------------------------------------------------------
  
  bool 
  runtime_fn_stub_t::unshift_argument (ptr<expr_t> e)
  {
    if (!_arglist) {
      _arglist = New refcounted<expr_list_t> ();
    }
    _arglist->push_front (e);
    return true;
  }

  //-----------------------------------------------------------------------

};

//
// end RFNs
//-----------------------------------------------------------------------
