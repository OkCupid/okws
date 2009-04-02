
#include "pub3.h"

// XDR functions for pub3 objects

//-----------------------------------------------------------------------

static void 
expr_to_rpc_ptr (const pub3::expr_t *e, rpc_ptr<xpub3_expr_t> *x)
{
  if (e) {
    x->alloc ();
    e->to_xdr (*x);
  }
}

//-----------------------------------------------------------------------

static void
expr_to_xdr (ptr<const pub3::expr_t> e, xpub3_expr_t *x)
{
  if (e) { e->to_xdr (x); }
  else   { x->set_typ (XPUB3_EXPR_NULL); }
}

//-----------------------------------------------------------------------

bool
pub3::for_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB3_FOR);
  x->forloop->lineno = lineno;
  x->forloop->iter = _iter;
  expr_to_xdr (_arr, &x->forloop->arr);
  if (_env) {
    _env->to_xdr (&x->forloop->body);
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
    _arr (expr_t::alloc (x.arr)),
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

pub3::include_t::include_t (const xpub3_include_t &x)
  : pfile_func_t (x.lineno),
    _file (expr_t::alloc (x.file)),
    _dict (expr_t::alloc (x.dict)) {}

//-----------------------------------------------------------------------

bool
pub3::include_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB3_INCLUDE);
  x->pub3_include->lineno = lineno;
  expr_to_xdr (_file, &x->pub3_include->file);
  expr_to_xdr (_dict, &x->pub3_include->dict);
  return true;
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
  case XPUB3_EXPR_ADD:
    r = New refcounted<pub3::expr_add_t> (*x.xadd);
    break;
  case XPUB3_EXPR_FN:
    r = pub3::rfn_factory_t::get ()->alloc (*x.fn);
    break;
  case XPUB3_EXPR_RELATION:
    r = New refcounted<pub3::expr_relation_t> (*x.relation);
    break;
  case XPUB3_EXPR_DICT:
    r = New refcounted<pub3::expr_dict_t> (*x.dict);
    break;
  case XPUB3_EXPR_LIST:
    r = New refcounted<pub3::expr_list_t> (*x.list);
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
    r = New refcounted<pub3::expr_varref_t> (*x.xref);
    break;
  case XPUB3_EXPR_STR:
    r = New refcounted<pub3::expr_str_t> (*x.xstr);
    break;
  case XPUB3_EXPR_SHELL_STR:
    r = New refcounted<pub3::expr_shell_str_t> (*x.shell_str);
    break;
  case XPUB3_EXPR_INT:
    r = New refcounted<pub3::expr_int_t> (*x.xint);
    break;
  case XPUB3_EXPR_UINT:
    r = New refcounted<pub3::expr_uint_t> (*x.xuint);
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
  : expr_logical_t (x.lineno),
    _t1 (expr_t::alloc (x.t1)),
    _t2 (expr_t::alloc (x.t2)) {}

//-----------------------------------------------------------------------

pub3::expr_AND_t::expr_AND_t (const xpub3_and_t &x)
  : expr_logical_t (x.lineno),
    _f1 (expr_t::alloc (x.f1)),
    _f2 (expr_t::alloc (x.f2)) {}

//-----------------------------------------------------------------------

pub3::expr_add_t::expr_add_t (const xpub3_add_t &x)
  : expr_arithmetic_t (x.lineno),
    _t1 (expr_t::alloc (x.t1)),
    _t2 (expr_t::alloc (x.t2)),
    _pos (x.pos) {}

//-----------------------------------------------------------------------

pub3::expr_NOT_t::expr_NOT_t (const xpub3_not_t &x)
  : expr_logical_t (x.lineno),
    _e (expr_t::alloc (x.e)) {}

//-----------------------------------------------------------------------

ptr<pub3::runtime_fn_t>
pub3::rfn_factory_t::alloc (const xpub3_fn_t &x)
{
  return alloc (x.name, expr_list_t::alloc (x.args), x.lineno);
}

//-----------------------------------------------------------------------

pub3::expr_list_t::expr_list_t (const xpub3_expr_list_t &x)
  : expr_t (x.lineno)
{
  setsize (x.list.size ());
  for (size_t i = 0; i < x.list.size (); i++) {
    (*this)[i] = pub3::expr_t::alloc (x.list[i]);
  }
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_LIST);
  return to_xdr (x->list);
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::to_xdr (xpub3_expr_list_t *l) const
{
  l->lineno = _lineno;
  l->list.setsize (size ());
  for (size_t i = 0; i < size (); i++) {
    ptr<const pub3::expr_t> x = (*this)[i];
    if (x) {
      x->to_xdr (&l->list[i]);
    } else {
      l->list[i].set_typ (XPUB3_EXPR_NULL);
    }
  }
  return true;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
pub3::expr_list_t::alloc (const xpub3_expr_list_t *x)
{
  ptr<expr_list_t> ret;
  if (x) ret = alloc (*x);
  return ret;
}

//-----------------------------------------------------------------------

pub3::expr_dictref_t::expr_dictref_t (const xpub3_dictref_t &x)
  : expr_ref_t (x.lineno),
    _dict (expr_t::alloc (x.dict)),
    _key (x.key) {}

//-----------------------------------------------------------------------

pub3::expr_vecref_t::expr_vecref_t (const xpub3_vecref_t &x)
  : expr_ref_t (x.lineno),
    _vec (expr_t::alloc (x.vec)),
    _index (expr_t::alloc (x.index)) {}

//-----------------------------------------------------------------------

pub3::expr_varref_t::expr_varref_t (const xpub3_ref_t &x)
  : expr_ref_t (x.lineno), _name (x.key) {}

//-----------------------------------------------------------------------

pub3::expr_relation_t::expr_relation_t (const xpub3_relation_t &x)
  : expr_logical_t (x.lineno),
    _l  (expr_t::alloc (x.left)),
    _r  (expr_t::alloc (x.right)),
    _op (x.relop) {}

//-----------------------------------------------------------------------

pub3::expr_EQ_t::expr_EQ_t (const xpub3_eq_t &x)
  : expr_logical_t (x.lineno),
    _o1  (expr_t::alloc (x.o1)),
    _o2  (expr_t::alloc (x.o2)),
    _pos (x.pos) {}

//-----------------------------------------------------------------------

pub3::expr_str_t::expr_str_t (const xpub3_str_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_int_t::expr_int_t (const xpub3_int_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_uint_t::expr_uint_t (const xpub3_uint_t &x)
  : _val (x.val) {}

//-----------------------------------------------------------------------

pub3::expr_double_t::expr_double_t (const xpub3_double_t &x)
  : _val (0)
{
  convertdouble (x.val, &_val);
}

//-----------------------------------------------------------------------

//-----------------------------------------------------------------------

bool
pub3::expr_AND_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_AND);
  x->xand->lineno = _lineno;
  expr_to_rpc_ptr (_f1, &x->xand->f1);
  expr_to_rpc_ptr (_f2, &x->xand->f2);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_OR_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_OR);
  x->xxor->lineno = _lineno;
  expr_to_rpc_ptr (_t1, &x->xxor->t1);
  expr_to_rpc_ptr (_t2, &x->xxor->t2);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_add_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_ADD);
  x->xadd->lineno = _lineno;
  expr_to_rpc_ptr (_t1, &x->xadd->t1);
  expr_to_rpc_ptr (_t2, &x->xadd->t2);
  x->xadd->pos = _pos;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_NOT_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_NOT);
  x->xnot->lineno = _lineno;
  expr_to_rpc_ptr (_e, &x->xnot->e);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::runtime_fn_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_FN);
  x->fn->lineno = _lineno;
  x->fn->name = name ();
  if (args ()) {
    args ()->to_xdr (&x->fn->args);
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_relation_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_RELATION);
  x->relation->lineno = _lineno;
  x->relation->relop = _op;
  expr_to_rpc_ptr (_l, &x->relation->left);
  expr_to_rpc_ptr (_r, &x->relation->right);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_EQ_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_EQ);
  x->eq->lineno = _lineno;
  x->eq->pos = _pos;
  expr_to_rpc_ptr (_o1, &x->eq->o1);
  expr_to_rpc_ptr (_o2, &x->eq->o2);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_dictref_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_DICTREF);
  x->dictref->lineno = _lineno;
  x->dictref->key = _key;
  expr_to_rpc_ptr (_dict, &x->dictref->dict);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_vecref_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_VECREF);
  x->vecref->lineno = _lineno;
  expr_to_rpc_ptr (_index, &x->vecref->index);
  expr_to_rpc_ptr (_vec, &x->vecref->vec);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_varref_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_REF);
  x->xref->lineno = _lineno;
  x->xref->key = _name;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_STR);
  x->xstr->val = _val;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_int_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_INT);
  x->xint->val = _val;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_uint_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_UINT);
  x->xuint->val = _val;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_double_t::to_xdr (xpub3_expr_t *x) const
{
#define BUFSZ 64
  x->set_typ (XPUB3_EXPR_DOUBLE);
  char buf[BUFSZ];
  snprintf (buf, BUFSZ, "%g", _val);
  x->xdouble->val = buf;
  return true;
#undef BUFSZ
}

//-----------------------------------------------------------------------

bool
pub3::cond_clause_t::to_xdr (xpub3_cond_clause_t *x) const
{
  x->lineno = _lineno;

  expr_to_xdr (_expr, &x->expr);
  if (_env) { _env->to_xdr (&x->body); }

  return true;
}

//-----------------------------------------------------------------------

bool
pub3::cond_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB3_COND);
  x->cond->lineno = lineno;

  size_t s = _clauses ? _clauses->size () : size_t (0);
  x->cond->clauses.setsize (s);
  for (size_t i = 0; i < s; i++) {
    (*_clauses)[i]->to_xdr (&x->cond->clauses[i]);
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_shell_str_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_SHELL_STR);
  x->shell_str->lineno = _lineno;
  if (_els) {
    _els->to_xdr (&x->shell_str->elements);
  }

  return true;
}

//-----------------------------------------------------------------------

pub3::expr_shell_str_t::expr_shell_str_t (const xpub3_shell_str_t &x)
  : expr_t (x.lineno),
    _els (expr_list_t::alloc (x.elements)) {}

//-----------------------------------------------------------------------

pub3::expr_dict_t::expr_dict_t (const xpub3_dict_t &x)
  : expr_t (x.lineno),
    _dict (New refcounted<aarr_arg_t> ())
{
  size_t lim = x.entries.size ();
  for (size_t i = 0; i < lim; i++) {
    const xpub3_nvpair_t &p = x.entries[i];
    _dict->add (New nvpair_t (p.key, expr_t::alloc (p.val)));
  }
}

//-----------------------------------------------------------------------

bool
pub3::expr_dict_t::to_xdr (xpub3_expr_t *x) const
{
  x->set_typ (XPUB3_EXPR_DICT);
  x->dict->lineno = _lineno;
  
  if (_dict) {
    const nvtab_t *tab = _dict->nvtab ();
    const nvpair_t *p;
    for (p = tab->first (); p; p = tab->next (p)) {
      ptr<const pub3::expr_t> e = p->value_expr ();
      if (e) {
	xpub3_nvpair_t &p3 = x->dict->entries.push_back ();
	p3.key = p->name ();
	expr_to_rpc_ptr (e, &p3.val);
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB3_EXPR);
  return to_xdr (x->pub3_expr);
}

//-----------------------------------------------------------------------

bool
pub3::inline_var_t::to_xdr (xpub_obj_t *x) const
{
  x->set_typ (XPUB3_INLINE_VAR);
  x->pub3_inline_var->lineno = _lineno;
  expr_to_xdr (_expr, &x->pub3_inline_var->expr);
  return true;
}

//-----------------------------------------------------------------------

pub3::inline_var_t::inline_var_t (const xpub3_inline_var_t &x)
  : _expr (expr_t::alloc (x.expr)),
    _lineno (x.lineno) {}

//-----------------------------------------------------------------------

bool
pub3::expr_t::to_xdr (xpub_val_t *x) const
{
  x->set_typ (XPUB3_VAL_EXPR);
  expr_to_rpc_ptr (this, x->expr);
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::set_func_t::to_xdr (xpub_obj_t *x) const
{
  return to_xdr_common (x, XPUB3_SET_FUNC);
}

//-----------------------------------------------------------------------


