// -*-c++-*-
#include "pub3func.h"

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

  ptr<runtime_fn_stub_t> 
  runtime_fn_stub_t::alloc (const str &n, ptr<expr_list_t> l)
  { return New refcounted<runtime_fn_stub_t> (n, l, plineno ()); }


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
