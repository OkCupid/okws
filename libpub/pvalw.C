
#include "pub.h"
#include "parr.h"

int
pval_w_t::to_int () const
{
  if (ival_flag) 
    return ival;
  const pval_t *v = val;
  if (!v && name)
    v = env->lookup (name, false);
  if (!v)
    return int_err;

  int64_t t;

  if (!v->to_int64 (&t)) 
    return int_err;

  if (t > INT_MAX || t < INT_MIN)
    return int_err;
  return int (t);
}

str
pval_w_t::to_str () const 
{
  ptr<pvar_t> pv;
  if (name) pv = New refcounted<pvar_t> (name) ;
  else if (val) pv =  New refcounted<pvar_t> (val);
  else if (ival_flag) return strbuf () << ival;
  else return NULL;
  return pv->eval (env, EVAL_INTERNAL);
}

const pval_t *
pval_w_t::get_pval () const
{
  if (val)
    return val;
  if (name)
    return env->lookup (name, false);
  return NULL;
}

pval_w_t 
pval_w_t::elem_at (u_int i) const
{
  const pval_t *v = get_pval ();
  if (!v)
    return pval_w_t ();
  const parr_mixed_t *m = v->to_mixed_arr ();
  if (m) {
    if (i < m->size ())
      return pval_w_t ((*m)[i], env);
    else
      return pval_w_t ();
  }
  const parr_ival_t *ia = v->to_int_arr ();
  if (ia) {
    int t;
    if (ia->val (i, &t) == PARR_OK)
      return pval_w_t (t);
  }
  return pval_w_t ();
}

u_int
pval_w_t::size () const
{
  const pval_t *v = get_pval ();
  const parr_t *a = v->to_arr ();
  return (a ? a->size () : 0);
}
