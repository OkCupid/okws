
#include "pub3.h"

// XDR functions for pub3 objects

//-----------------------------------------------------------------------

bool
pub3::for_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB_FOR);
  x->forloop->lineno = lineno;
  x->forloop->iter = _iter;
  x->forloop->arr = _arr;
  if (_env && _env->sec ()) {
    _env->sec ()->to_xdr (&x->forloop->body);
  }
  if (_empty && _empty->sec ()) {
    _empty->sec ()->to_xdr (&x->forloop->empty);
  }
  return true;
}

//-----------------------------------------------------------------------

pub3::for_t::for_t (const xpub3_for_t &x)
  : pfile_func_t (x.lineno),
    _iter (x.iter),
    _arr (x.arr),
    _env (nested_env_t::alloc (x.body)),
    _empty (nested_env_t::alloc (x.empty)) {}

//-----------------------------------------------------------------------

pub3::cond_clause_t::cond_clause_t (const xpub3_cond_clause_t &x)
  : _lineno (x.lineno),
    _expr (expr_t::alloc (x.expr)),
    _env (nested_env_t::alloc (x.body)) {}

//-----------------------------------------------------------------------

pub3::cond_t::cond_t (const xpub3_cond_t &x)
  : pfile_func_t (x.lineno)
{
  if (x.clauses.size ()) {
    _clauses = New refcounted<cond_clause_list_t> ();
    for (size_t i = 0; i < x.clauses.size (); i++) {
      _clauses->push_back (New refcounted<cond_clause_t> (x.clauses[i]));
    }
  }
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_t::alloc (const xpub3_expr_t *x)
{
  ptr<pub3::expr_t> ret;
  if (x) ret = expr_t::alloc (*x);
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_t::alloc (const xpub3_expr_t &x)
{
  ptr<pub3::expr_t> r;
  switch (x.typ) {
  case XPUB3_EXPR_OR:
    r = New refcounted<pub3::expr_OR_t> (*x.xxor);
    break;
  case XPUB3_EXPR_AND:
    r = New refcounted<pub3::expr_AND_t> (*x.xand);
    break;
  case XPUB3_EXPR_NOT:
    r = New refcounted<pub3::expr_NOT_t> (*x.xnot);
    break;
  case XPUB3_EXPR_FN:
    r = pub3::runtime_fn_t::alloc (*x.fn);
    break;
  default:
    break;
  }
  return r;
}

//-----------------------------------------------------------------------

pub3::expr_OR_t::expr_OR_t (const xpub3_or_t &x)
  : _t1 (expr_t::alloc (x.t1)),
    _t2 (expr_t::alloc (x.t2)) {}

//-----------------------------------------------------------------------

pub3::expr_AND_t::expr_AND_t (const xpub3_and_t &x)
  : _f1 (expr_t::alloc (x.f1)),
    _f2 (expr_t::alloc (x.f2)) {}

//-----------------------------------------------------------------------

pub3::expr_NOT_t::expr_NOT_t (const xpub3_not_t &x)
  : _e (expr_t::alloc (x.e)) {}

//-----------------------------------------------------------------------


ptr<pub3::runtime_fn_t>
pub3::runtime_fn_t::alloc (const xpub3_fn_t &x)
{
  return alloc (x.name, expr_t::alloc (x.args), x.lineno, NULL);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
pub3::expr_t::alloc (const xpub3_expr_list_t &x)
{
  ptr<expr_list_t> ret = New refcounted<expr_list_t> ();
  for (size_t i = 0; i < x.size (); i++) {
    ret->push_back (expr_t::alloc (x[i]));
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
pub3::expr_t::alloc (const xpub3_expr_list_t *x)
{
  ptr<expr_list_t> ret;
  if (x) ret = alloc (*x);
  return ret;
}

//-----------------------------------------------------------------------
