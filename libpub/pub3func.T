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

    if (_empty) {
      twait { _empty->publish (iface, o, e, mkevent (res)); }
    }

  } else {

    assert (v);

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
	ptr<expr_dict_t> tmp = row_dict->copy_stub_dict ();
	tmp->replace ("count", expr_int_t::alloc (len));
	tmp->replace ("iter", expr_int_t::alloc (i));
	tmp->replace ("last", expr_bool_t::alloc (len - 1 == i));
	tmp->replace ("first", expr_bool_t::alloc (i == 0));
	tmp->replace ("odd", expr_bool_t::alloc (i % 2));
	tmp->replace ("even", expr_bool_t::alloc (1 - (i % 2)));
	row_val = tmp;
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

void
pub3::cond_t::add_clauses (ptr<cond_clause_list_t> ccl)
{
  if (!_clauses) {
    _clauses = ccl;
  } else if (ccl) {
    for (size_t i = 0; i < ccl->size (); i++) {
      _clauses->push_back ((*ccl)[i]);
    }
  }
}

//-----------------------------------------------------------------------

void
pub3::cond_t::add_clause (ptr<cond_clause_t> cc)
{
  if (cc) {
    if (!_clauses) {
      _clauses = New refcounted<cond_clause_list_t> ();
    }
    _clauses->push_back (cc);
  }
}

//-----------------------------------------------------------------------

bool
pub3::cond_t::publish_nonblock (pub2_iface_t *i, output_t *o, penv_t *e) const
{
  bool ret = false;

  // XXX - can do better than this.  This means we'll go into blocking
  // land if ANY clause of the if statement blocks.  It should be
  // that we'll go into blocking code only if the selected clause needs
  // to block.
  if (!might_block ()) {
    ptr<nested_env_t> ne = find_clause (o, e);
    if (ne) {
      ret = ne->publish_nonblock (i, o, e);
    } else {
      ret = true;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<nested_env_t>
pub3::cond_t::find_clause (output_t *o, penv_t *e) const
{
  size_t len = _clauses ? _clauses->size () : size_t (0) ;
  ptr<nested_env_t> ret;
  bool found = false;
  pub3::eval_t eval (e, o);
  ptr<const cond_clause_t> clause;
  ptr<const expr_t> x;

  for (size_t i = 0; !found&& i < len; i++) {
    
    // Note that expr will be NULL for the last (else) clause if
    // one was there....
    if ((clause = (*_clauses)[i]) &&
	(!(x = clause->expr ()) || (x && x->eval_as_bool (eval)))) {
      
      found = true;
      ret = clause->env ();
    }
  }
    return ret;
}

//-----------------------------------------------------------------------

tamed void
pub3::cond_t::publish (pub2_iface_t *iface, output_t *o, penv_t *e,
		      xpub_status_cb_t callercv) const
{
  tvars {
    xpub_status_t res (XPUB_STATUS_OK);
    ptr<nested_env_t> env;
  }

  if ((env = find_clause (o, e))) {
    twait { env->publish (iface, o, e, mkevent (res)); }
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
pub3::set_func_t::output (output_t *o, penv_t *e) const
{
  aarr_t *odd; // output dict dest
  ptr<aarr_t> edd; // env dict dest
  aarr_t tmp;
  
  eval_t eval (e, o);

  odd = o->dict_dest ();
  edd = e->get_global_aarr ();

  if (odd || edd) {
    eval.eval_freeze_dict (aarr, &tmp);
  }

  if (odd) { odd->overwrite_with (tmp); }
  if (edd) { edd->overwrite_with (tmp); }

  if (!edd) {
    e->push (aarr);
  }
}

//-----------------------------------------------------------------------

void
pub3::cond_t::output (output_t *o, penv_t *e) const
{
}

//-----------------------------------------------------------------------

bool
pub3::cond_clause_t::might_block () const
{
  return (_env && _env->might_block ());
}

//-----------------------------------------------------------------------

bool
pub3::cond_t::might_block () const
{
  if (_might_block < 0) {
    _might_block = 0;

    for (size_t i = 0; 
	 _might_block == 0 && _clauses && i < _clauses->size (); 
	 i++) {

      if ((*_clauses)[i]->might_block ()) { _might_block = 1; }
    }

  }
  return (_might_block);
}

//-----------------------------------------------------------------------

bool
pub3::expr_statement_t::publish_nonblock (pub2_iface_t *i, output_t *o, 
					  penv_t *e) const
{
  pub3::eval_t evl (e, o);
  if (_expr) {
    (void)_expr->eval (evl);
  }
  return true;
}
  
//-----------------------------------------------------------------------

void 
pub3::expr_statement_t::output (output_t *o, penv_t *e) const
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
