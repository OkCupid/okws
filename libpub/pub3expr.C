
#include "pub3expr.h"

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
    case REL_LT:  ret = (l < r); break;
    case REL_GT:  ret = (l > r); break;
    case REL_LTE: ret = (l <= r); break;
    case REL_GTE: ret = (l >= r); break;
    default: panic ("unexpected relational operator!\n");
    }
  }
  return ret;
}

//-----------------------------------------------------------------------
