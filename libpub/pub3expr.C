
#include "pub3expr.h"
#include "parseopt.h"

//-----------------------------------------------------------------------

bool
pub3::expr_OR_t::eval_as_bool (penv_t *e) const
{
  return ((_t1 && _t1->eval_as_bool (e)) || (_t2 && _t2->eval_as_bool (e)));
}

//-----------------------------------------------------------------------

bool
pub3::expr_AND_t::eval_as_bool (penv_t *e) const
{
  return ((_f1 && _f1->eval_as_bool (e)) && (_f2 && _f2->eval_as_bool (e)));
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_logical_t::eval_as_scalar (penv_t *e) const
{
  scalar_obj_t o;
  o.set (eval_as_int (e));
  return o;
}

//-----------------------------------------------------------------------

str
pub3::expr_logical_t::eval_as_str (penv_t *e) const
{
  bool b = eval_as_bool (e);
  return b ? "True" : "False";
}

//-----------------------------------------------------------------------

bool
pub3::expr_EQ_t::eval_as_bool (penv_t *e) const
{
  int flip = _pos ? 0 : 1;
  int tmp;
  bool n1 = !_o1 || _o1->is_null (e);
  bool n2 = !_o2 || _o2->is_null (e);

  if (n1 && n2) { tmp = 1; }
  else if (n1) { tmp = 0; }
  else if (n2) { tmp = 0; }
  else {
    scalar_obj_t o1 = _o1->eval_as_scalar (e);
    scalar_obj_t o2 = _o2->eval_as_scalar (e);
    tmp = (o1 == o2) ? 1 : 0;
  }

  return (tmp ^ flip);
}

//-----------------------------------------------------------------------

bool
pub3::expr_relational_t::eval_as_bool (penv_t *e) const
{
  bool ret = false;
  if (_l && !_l->is_null (e) && _r && !_r->is_null (e)) {
    int64_t l = _l->eval_as_int (e);
    int64_t r = _r->eval_as_int (e);
    switch (_op) {
    case XPUB3_REL_LT : ret = (l < r);  break;
    case XPUB3_REL_GT : ret = (l > r);  break;
    case XPUB3_REL_LTE: ret = (l <= r); break;
    case XPUB3_REL_GTE: ret = (l >= r); break;
    default: panic ("unexpected relational operator!\n");
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_NOT_t::eval_as_bool (penv_t *e) const
{
  bool ret = true;
  if (_e) ret = !(_e->eval_as_bool (e));
  return ret;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_dictref_t::eval_as_pval (penv_t *e) const
{
  ptr<const aarr_t> d;
  ptr<const pval_t> v;
  if (_dict && (d = _dict->eval_as_dict (e))) {
    v = d->lookup_ptr (_key);
  }
  return v;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_vecref_t::eval_as_pval (penv_t *e) const
{
  ptr<const parr_mixed_t> v;
  ptr<const pval_t> r;
  int64_t i;

  if (_vec && (v = _vec->eval_as_vec (e))) {
    if (_index) i = _index->eval_as_int (e);
    r = (*v)[i];
  }
  return r;
}


//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_t::eval_as_scalar (penv_t *e) const
{
  scalar_obj_t so;
  ptr<const pval_t> v = eval_as_pval (e);
  ptr<const pub_scalar_t> s;
  if (v && (s = v->to_scalar ())) {
    so = s->obj ();
  }
  return so;
}

//-----------------------------------------------------------------------

bool
pub3::expr_t::eval_as_bool (penv_t *e) const
{
  scalar_obj_t so = eval_as_scalar (e);
  return so.to_bool ();
}

//-----------------------------------------------------------------------

int64_t
pub3::expr_t::eval_as_int (penv_t *e) const
{
  scalar_obj_t so = eval_as_scalar (e);
  return so.to_int64 ();
}

//-----------------------------------------------------------------------

u_int64_t
pub3::expr_t::eval_as_uint (penv_t *e) const
{
  scalar_obj_t so = eval_as_scalar (e);
  return so.to_uint64 ();
}

//-----------------------------------------------------------------------

str
pub3::expr_t::eval_as_str (penv_t *e) const
{
  scalar_obj_t so = eval_as_scalar (e);
  return so.to_str ();
}

//-----------------------------------------------------------------------

bool
pub3::expr_t::is_null (penv_t *e) const
{
  return eval_as_pval (e) == NULL;
}

//-----------------------------------------------------------------------

ptr<const aarr_t>
pub3::expr_t::eval_as_dict (penv_t *e) const
{
  ptr<const aarr_arg_t> r;
  ptr<const pval_t> v;
  if ((v = eval_as_pval (e))) 
    r = v->to_aarr ();
  return r;
}

//-----------------------------------------------------------------------

ptr<const parr_mixed_t>
pub3::expr_t::eval_as_vec (penv_t *e) const
{
  ptr<const parr_mixed_t> r;
  ptr<const pval_t> v;
  if ((v = eval_as_pval (e))) 
    r = v->to_mixed_arr ();
  return r;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_ref_t::eval_as_pval (penv_t *e) const
{
  const pval_t *v = e->lookup (_name, false);
  ptr<const pval_t> ret;
  if (!v && e->debug ()) {
    e->setlineno (_lineno);
    e->warning (strbuf ("cannot resolve variable: " ) << _name.cstr ());
    e->unsetlineno ();
  } else {
    ret = mkref (v);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::eval_as_bool (penv_t *e) const
{
  return (_val && _val.len ());
}

//-----------------------------------------------------------------------

int64_t
pub3::expr_str_t::eval_as_int (penv_t *e) const
{
  int64_t ret = 0;
  if (_val)
    convertint (_val, &ret);
  return ret;
}

//-----------------------------------------------------------------------

str
pub3::expr_str_t::eval_as_str (penv_t *e) const
{
  return _val;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_str_t::eval_as_scalar (penv_t *e) const
{
  return scalar_obj_t (_val);
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_str_t::eval_as_pval (penv_t *e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar (e));
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::is_null (penv_t *e) const
{
  return !_val;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_int_t::eval_as_scalar (penv_t *e) const
{
  scalar_obj_t so;
  so.set (_val);
  return so;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_int_t::eval_as_pval (penv_t *e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar (e));
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_double_t::eval_as_scalar (penv_t *e) const
{
  scalar_obj_t so;
  so.set (_val);
  return so;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_double_t::eval_as_pval (penv_t *e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar (e)); 
}

//-----------------------------------------------------------------------
