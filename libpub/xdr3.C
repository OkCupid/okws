
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
  case XPUB3_EXPR_AND:
    r = New refcounted<pub3::expr_AND_t> (*x.xand);
    break;
  case XPUB3_EXPR_OR:
    r = New refcounted<pub3::expr_OR_t> (*x.xxor);
    break;
  case XPUB3_EXPR_NOT:
    r = New refcounted<pub3::expr_NOT_t> (*x.xnot);
    break;
  case XPUB3_EXPR_FN:
    r = pub3::runtime_fn_t::alloc (*x.fn);
    break;
  case XPUB3_EXPR_RELATION:
    r = New refcounted<pub3::expr_relation_t> (*x.relation);
    break;
  case XPUB3_EXPR_EQ:
    r = New refcounted<pub3::expr_EQ_t> (*x.eq);
    break;
  case XPUB3_EXPR_DICTREF:
    r = New refcounted<pub3::expr_dictref_t> (*x.dictref);
    break;
  case XPUB3_EXPR_VECREF:
    r = New refcounted<pub3::expr_vecref_t> (*x.vecref);
    break;
  case XPUB3_EXPR_REF:
    r = New refcounted<pub3::expr_ref_t> (*x.xref);
    break;
  case XPUB3_EXPR_STR:
    r = New refcounted<pub3::expr_str_t> (*x.xstr);
    break;
  case XPUB3_EXPR_INT:
    r = New refcounted<pub3::expr_int_t> (*x.xint);
    break;
  case XPUB3_EXPR_DOUBLE:
    r = New refcounted<pub3::expr_double_t> (*x.xdouble);
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

pub3::expr_dictref_t::expr_dictref_t (const xpub3_dictref_t &x)
  : _dict (expr_t::alloc (x.dict)),
    _key (x.key) {}

//-----------------------------------------------------------------------

pub3::expr_vecref_t::expr_vecref_t (const xpub3_vecref_t &x)
  : _vec (expr_t::alloc (x.vec)),
    _index (expr_t::alloc (x.index)) {}

//-----------------------------------------------------------------------

pub3::expr_ref_t::expr_ref_t (const xpub3_ref_t &x)
  : _name (x.key), _lineno (x.lineno) {}

//-----------------------------------------------------------------------

pub3::expr_relation_t::expr_relation_t (const xpub3_relation_t &x)
  : _l  (expr_t::alloc (x.left)),
    _r  (expr_t::alloc (x.right)),
    _op (x.relop),
    _lineno (x.lineno) {}

//-----------------------------------------------------------------------

pub3::expr_EQ_t::expr_EQ_t (const xpub3_eq_t &x)
  : _o1  (expr_t::alloc (x.o1)),
    _o2  (expr_t::alloc (x.o2)),
    _pos (x.pos),
    _lineno (x.lineno) {}

//-----------------------------------------------------------------------

pub3::expr_str_t::expr_str_t (const xpub3_str_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_int_t::expr_int_t (const xpub3_int_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_double_t::expr_double_t (const xpub3_double_t &x)
  : _val (0)
{
  convertdouble (x.val, &_val);
}


//-----------------------------------------------------------------------
