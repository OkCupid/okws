
#include "pub3expr.h"
#include "parseopt.h"
#include "okformat.h"
#include "pub3func.h"
#include "pescape.h"
#include "precycle.h"
#include "pub3eval.h"

//-----------------------------------------------------------------------

static size_t depth;
#define DEBUG_ENTER()					\
  do {							\
  if (0) {						\
    strbuf b;						\
    for (size_t i = 0; i <= depth; i++) { b << "+"; }	\
    b << " " << __FUNCTION__ << "\n";			\
    warn << b;						\
    depth ++;						\
  }							\
  } while (0)

#define DEBUG_EXIT(r)					\
  do {							\
  if (0) {						\
    strbuf b;						\
    for (size_t i = 0; i <= depth; i++) { b << "-"; }	\
    b << " " << __FUNCTION__ << " -> " << r << "\n";	\
    warn << b;						\
    depth --;						\
  }							\
  } while (0)


//-----------------------------------------------------------------------

str pub3::json::_null = "null";

//-----------------------------------------------------------------------

str 
pub3::json::safestr (const str &s)
{
  str ret;
  if (!s) {
    ret = _null;
  } else {
    ret = s;
  }
  return ret;
}

//-----------------------------------------------------------------------

str
pub3::json::quote (const str &s)
{
  str ret;
  if (!s) {
    ret = _null;
  } else {
    ret = json_escape (s, true);
  }
  return ret;
}

//-----------------------------------------------------------------------

static ptr<pub3::expr_null_t> g_null;

ptr<pub3::expr_null_t>
pub3::expr_null_t::alloc (int l)
{
  ptr<pub3::expr_null_t> ret;
  if (l == -1) {
    if (!g_null) {
      g_null = New refcounted<expr_null_t> ();
    }
    ret = g_null;
  } else {
    ret = New refcounted<expr_null_t> (l);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_t::eval_freeze (eval_t e) const
{
  ptr<pval_t> r;
  ptr<const pval_t> v = eval (e);
  if (v) r = v->copy_stub ();
  return r;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_bool_t::to_scalar () const
{
  scalar_obj_t so;
  so.set_i (_b);
  return so;
}

//-----------------------------------------------------------------------

str
pub3::expr_bool_t::to_str (bool b)
{
  return b ? "true" : "false";
}

//-----------------------------------------------------------------------

str
pub3::expr_bool_t::to_str () const
{
  return to_str (_b);
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_logical_t::eval (eval_t e) const
{
  if (_cache_generation != e.cache_generation ()) {
    _cache_generation = e.cache_generation ();
    _cached_bool = eval_internal (e);
    _cached_val = expr_bool_t::alloc (_cached_bool);
  }
  return _cached_val;
}

//-----------------------------------------------------------------------

bool
pub3::expr_ref_t::eval_as_bool (eval_t e) const
{
  bool q = e.set_silent (true);
  ptr<const pval_t> v = eval_internal (e);

  bool ret = false;

  if (v && !v->is_null ()) {
    ret = v->to_bool ();
  }

  e.set_silent (q);
  return ret;
}


//-----------------------------------------------------------------------

bool
pub3::expr_ref_t::eval_as_null (eval_t e) const
{
  bool q = e.set_silent (true);
  ptr<const pval_t> v = eval_internal (e);
  bool ret = v ? v->is_null () : true;
  e.set_silent (q);
  return ret;
}

//-----------------------------------------------------------------------

ptr<rxx>
pub3::expr_ref_t::eval_as_regex (eval_t e) const
{
  ptr<rxx> ret;
  ptr<const pval_t> v = eval_internal (e);
  if (v) ret = v->to_regex ();
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_OR_t::eval_internal (eval_t e) const
{
  bool ret = false;
  ret = ((_t1 && _t1->eval_as_bool (e)) || (_t2 && _t2->eval_as_bool (e)));
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_AND_t::eval_internal (eval_t e) const
{
  bool ret;
  ret = ((_f1 && _f1->eval_as_bool (e)) && (_f2 && _f2->eval_as_bool (e)));
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_EQ_t::eval_internal (eval_t e) const
{
  bool ret = false;

  int flip = _pos ? 0 : 1;
  int tmp;
  bool n1 = !_o1 || _o1->eval_as_null (e);
  bool n2 = !_o2 || _o2->eval_as_null (e);
  
  
  if (n1 && n2) { tmp = 1; }
  else if (n1) { tmp = 0; }
  else if (n2) { tmp = 0; }
  else {
    scalar_obj_t o1 = _o1->eval_as_scalar (e);
    scalar_obj_t o2 = _o2->eval_as_scalar (e);
    tmp = (o1 == o2) ? 1 : 0;
  } 
  ret = (tmp ^ flip);

  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_relation_t::eval_internal (eval_t e) const
{
  bool ret = false;
  if (_l && !_l->eval_as_null (e) && _r && !_r->eval_as_null (e)) {
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
pub3::expr_NOT_t::eval_internal (eval_t e) const
{
  bool ret = true;
  if (_e) ret = !(_e->eval_as_bool (e));
  return ret;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_dictref_t::deref_step (eval_t *e) const
{
  ptr<const aarr_t> d;
  ptr<const pval_t> v;
  if (!_dict) {
    report_error (*e, "dict reference into NULL");
  } else if (!(d = _dict->eval_as_dict (*e))) {
    report_error (*e, "dict reference into non-dict");
  } else if (!(v = d->lookup_ptr (_key)) && e->loud ()) {
    strbuf b ("cannot resolve key '%s'", _key.cstr ());
    report_error (*e, b);
  }
  return v;
}

//-----------------------------------------------------------------------

ptr<slot_ref_t>
pub3::expr_dictref_t::lhs_deref_step (eval_t *e)
{
  ptr<const aarr_t> d;
  aarr_t *dm;
  ptr<slot_ref_t> r;

  if (!_dict) {
    report_error (*e, "dict reference into NULL");
  } else if (!(d = _dict->eval_as_dict (*e))) {
    report_error (*e, "dict reference into non-dict");
  } else if (!(dm = d->const_cast_hack ())) {
    report_error (*e, "hacked const cast failed");
  } else if (!(r = dm->lookup_slot (_key)) && e->loud ()) {
    strbuf b ("cannot resolve key '%s'", _key.cstr ());
    report_error (*e, b);
  }
  return r;
}

//-----------------------------------------------------------------------

ptr<slot_ref_t>
pub3::expr_ref_t::lhs_deref (eval_t *e)
{
  ptr<slot_ref_t> ret;
  ptr<expr_ref_t> p = mkref (this);
  ptr<expr_t> tmp;
  while (p && (ret = p->lhs_deref_step (e)) && (tmp = ret->deref_expr ())) {
    p = tmp->to_ref ();
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_ref_t::deref (eval_t *e) const
{
  ptr<const expr_ref_t> p = mkref (this);
  ptr<const pval_t> r;

  while (p && (r = p->deref_step (e))) {
    p = r->to_ref (); 
  }
  return r;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_ref_t::eval (eval_t e) const
{
  return eval_internal (e);
}

//-----------------------------------------------------------------------

str
pub3::expr_ref_t::eval_as_str (eval_t e) const
{
  str ret;
  ptr<const pval_t> v;
  ptr<const expr_t> x;

  if (!(v = deref (&e))) {
    /* failed to deref -- warn? */
  } else if (!(x = v->to_expr ())) {

    eval_t *old = e.link_to_penv ();
    ret = v->eval (e.penv (), EVAL_INTERNAL, true);
    e.unlink_from_penv (old);

  } else {
    ret = x->eval_as_str (e);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_ref_t::eval_freeze (eval_t e) const
{
  ptr<pval_t> ret;
  ptr<const pval_t> v = eval_internal (e);
  if (v) ret = v->copy_stub ();
  return ret;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_ref_t::eval_internal (eval_t e) const
{
  ptr<const pval_t> v;
  ptr<const expr_t> x;

  if (!(v = deref (&e))) {
    /* failed to deref -- warn? */
  } else if (!(x = v->to_expr ())) {
    /* leave it as if it's a v1 or v2 pval */
  } else {
    /* if it's a full expression, we might still need to evaluate it */
    v = x->eval (e);
  }
  return v;
}

//-----------------------------------------------------------------------

ptr<slot_ref_t>
pub3::expr_vecref_t::lhs_deref_step (eval_t *e) 
{
  ptr<const parr_mixed_t> v;
  ptr<const pval_t> pvv;
  ptr<const expr_list_t> el;
  ptr<const vec_iface_t> vif;
  ptr<const aarr_t> dict;
  ptr<slot_ref_t> r;
  bool evr;

  vec_iface_t *mvif = NULL;
  aarr_t *mdict = NULL;

  scalar_obj_t key;

  if (!_index) {
    report_error (*e, "list index is not defined");
  } else if (!_vec) {
    if (e->loud ()) report_error (*e, "list is undefined");
  } else if ((key = _index->eval_as_scalar (*e)).is_null ()) {
    report_error (*e, "key to dictionary/list is undefined");
  } else if (!(evr = _vec->eval_as_vec_or_dict (*e, &vif, &dict))) {
    report_error (*e, "list reference into non-list");
  }

  if (vif) mvif = vif->const_cast_hack ();
  else if (dict) mdict = dict->const_cast_hack ();

  if (mvif) {
    size_t i = key.to_int ();
    if (!(r = mvif->lookup_slot (i)) && e->loud ()) {
      report_error (*e, strbuf ("list reference (%zd) out of bounds", i));
    }
  } else if (mdict) {
    str k = key.to_str ();
    if (!k) {
      report_error (*e, "cannot resolve dict key");
    } else if (!(r = mdict->lookup_slot (k)) && e->loud ()) {
      report_error (*e, strbuf ("cannot resolve key '%s'", k.cstr ()));
    }
  }

  return r;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_vecref_t::deref_step (eval_t *e) const
{
  ptr<const parr_mixed_t> v;
  ptr<const pval_t> pvv, r;
  ptr<const expr_list_t> el;
  ptr<const vec_iface_t> vif;
  ptr<const aarr_t> dict;
  bool in_bounds;

  
  if (!_vec) {
    if (e->loud ()) report_error (*e, "list is undefined");
  } else if (!_vec->eval_as_vec_or_dict (*e, &vif, &dict)) {
    report_error (*e, "list reference into non-list");
  } else if (vif) {
    ssize_t i = 0;
    if (_index) {
      i = _index->eval_as_int (*e);
    }
    if (!(r = vif->lookup (i, &in_bounds)) && !in_bounds && e->loud ()) {
      report_error (*e, strbuf ("list reference (%zd) out of bounds", i));
    }
  } else {
    assert (dict);
    str k = _index->eval_as_str (*e);
    if (!k) {
      report_error (*e, "cannot resolve dict key");
    } else if (!(r = dict->lookup_ptr (k)) && e->loud ()) {
      report_error (*e, strbuf ("cannot resolve key '%s'", k.cstr ()));
    }
  }
  return r;
}

//-----------------------------------------------------------------------

void
pub3::expr_t::report_error (eval_t e, str msg) const
{
  penv_t *env = e.penv ();
  output_t *out = e.output ();
  env->setlineno (_lineno);
  env->warning (msg);
  if (out) {
    out->output_err (env, msg);
  }
  env->unsetlineno ();
}

//-----------------------------------------------------------------------

ptr<slot_ref_t>
pub3::expr_varref_t::lhs_deref_step (eval_t *e)
{
  ptr<slot_ref_t> ret;
  ret = e->lhs_resolve (this, _name);
  return ret;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_varref_t::deref_step (eval_t *e) const
{
  ptr<const pval_t> ret;
  ret = e->resolve (this, _name);
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::to_bool () const
{
  return (_val && _val.len ());
}

//-----------------------------------------------------------------------

str
pub3::expr_str_t::to_str () const
{
  return _val;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_str_t::to_scalar () const
{
  DEBUG_ENTER ();
  scalar_obj_t so = scalar_obj_t (_val);
  DEBUG_EXIT ("");
  return so;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::eval_as_null (eval_t e) const
{
  return to_null ();
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::to_null () const
{
  return !_val;
}

//-----------------------------------------------------------------------

static recycler_t<pub3::expr_int_t> _int_recycler (1000);
static recycler_t<pub3::expr_bool_t> _bool_recycler (1000);

//-----------------------------------------------------------------------

ptr<pub3::expr_int_t>
pub3::expr_int_t::alloc (int64_t i)
{
  return _int_recycler.alloc (i);
}

//-----------------------------------------------------------------------

void
pub3::expr_int_t::finalize ()
{
  _int_recycler.recycle (this);
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_int_t::to_scalar () const
{
  DEBUG_ENTER ();
  scalar_obj_t so;
  so.set (_val);
  DEBUG_EXIT ("");
  return so;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_double_t::to_scalar () const
{
  scalar_obj_t so;
  so.set (_val);
  return so;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_uint_t::to_scalar () const
{
  scalar_obj_t so;
  so.set_u (_val);
  return so;
}

//-----------------------------------------------------------------------

bool
pub3::eval_t::set_loud (bool b)
{
  bool c = _loud;
  _loud = b;
  return c;
}

//-----------------------------------------------------------------------

bool
pub3::eval_t::set_silent (bool b)
{
  bool c = _silent;
  _silent = b;
  return c;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_arithmetic_t::eval_as_scalar (eval_t e) const
{
  if (_cache_generation != e.cache_generation ()) {
    _so = eval_internal (e);
    _cache_generation = e.cache_generation ();
  }
  return _so;
}

//-----------------------------------------------------------------------

ptr<const pval_t> 
pub3::expr_arithmetic_t::eval (eval_t e) const
{
  return eval_freeze (e);
}

//-----------------------------------------------------------------------

ptr<pval_t> 
pub3::expr_arithmetic_t::eval_freeze (eval_t e) const
{
  ptr<expr_t> r = eval_as_frozen_list (e);
  if (!r) {
    scalar_obj_t so = eval_as_scalar (e);
    r = expr_t::alloc (so);
  }
  return r;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_t::alloc (scalar_obj_t so)
{
  ptr<expr_t> ret;
  int64_t i;
  u_int64_t u;
  str s;

  if (so.to_int64 (&i)) {
    ret = expr_int_t::alloc (i);
  } else if (so.to_uint64 (&u)) {
    ret = New refcounted<expr_uint_t> (u);
  } else if ((s = so.to_str ())) {
    ret = New refcounted<expr_str_t> (s);
  } else {
    ret = New refcounted<expr_null_t> ();
  }

  return ret;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_mod_t::eval_internal (eval_t e) const
{
  scalar_obj_t out;
  if (_n && !_n->eval_as_null (e) && _d && !_d->eval_as_null (e)) {
    bool l = e.set_loud (true);
    scalar_obj_t sn = _n->eval_as_scalar (e);
    scalar_obj_t sd = _d->eval_as_scalar (e);
    e.set_loud (l);
    bool zed = false;

    int64_t n, d;
    u_int64_t un, ud;

    if (sn.to_uint64 (&un) && sd.to_uint64 (&ud)) {
      if (ud == 0) { zed = true; }
      else { out.set_u (un % ud); }
    } else if (sn.to_int64 (&n) && sd.to_uint64 (&ud)) {
      if (ud == 0) { zed = true; }
      else { out.set_u (n % ud); }
    } else if (sn.to_uint64 (&un) && sd.to_int64 (&d)) {
      if (d == 0) { zed = true; }
      else { out.set_i (un % d); }
    } else if (sn.to_int64 (&n) && sd.to_int64 (&d)) {
      if (d == 0) { zed = true; }
      else { out.set_i (n % d); }
    }

    if (zed) {
      report_error (e, "refused to mod by 0!");
    }
  } else {
    report_error (e, "modulo: one or more operands were NULL");
  }
  return out;
}

//-----------------------------------------------------------------------

scalar_obj_t 
pub3::expr_div_t::eval_internal (eval_t e) const
{
  scalar_obj_t out;

  if (_n && !_n->eval_as_null (e) && _d && !_d->eval_as_null (e)) {
    bool l = e.set_loud (true);
    scalar_obj_t n = _n->eval_as_scalar (e);
    scalar_obj_t d = _d->eval_as_scalar (e);
    e.set_loud (l);
    int64_t id, in;
    u_int64_t un;

    if (!d.to_int64 (&id) || id == 0) {
      report_error (e, "refusing to divide by 0");
    } else if (n.to_int64 (&in)) {
      out.set_i (in / id);
    } else if (n.to_uint64 (&un)) {
      out.set_i (un / id);
    }
  } else {
    report_error (e, "division: one or more operands were NULL");
  }
  return out;
}

//-----------------------------------------------------------------------

scalar_obj_t 
pub3::expr_mult_t::eval_internal (eval_t e) const
{
  scalar_obj_t out;

  if (_f1 && !_f1->eval_as_null (e) && _f2 && !_f2->eval_as_null (e)) {
    bool l = e.set_loud (true);
    scalar_obj_t o1 = _f1->eval_as_scalar (e);
    scalar_obj_t o2 = _f2->eval_as_scalar (e);
    e.set_loud (l);
    int64_t i1, i2;
    u_int64_t u1, u2;
    str s1, s2;
    if (o1.to_int64 (&i1) && o2.to_int64 (&i2)) {
      out.set_i (i1*i2);
    } else if (o1.to_uint64 (&u1) && o2.to_uint64 (&u2)) {
      out.set_u (u1 * u2);
    } else if (o1.to_int64 (&i1) && o2.to_uint64 (&u2)) {
      out.set_i (u1 * u2);
    } else if (o1.to_uint64 (&u1) && o2.to_int64 (&i2)) {
      out.set_i (u1 * u2);
    } else if ((s1 = o1.to_str ()) && o2.to_int64 (&i2) &&
	       i2 < 0x100) {
      strbuf b;
      for (int64_t i = 0; i < i2; i++) {
	b << s1;
      }
      out.set (b);
    } else {
      report_error (e, "mult: no plausible evaluation found!");
    }
  } else {
    report_error (e, "mult: one or more operands were NULL");
  }

  return out;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
pub3::expr_add_t::eval_as_frozen_list (eval_t e) const
{
  ptr<expr_list_t> out;
  if (_t1 && !_t1->eval_as_null (e) && _t2 && !_t2->eval_as_null (e)) {
    ptr<pval_t> v1, v2;
    ptr<expr_list_t> l1, l2;
    if (!(v1 = _t1->eval_freeze (e)) || !(l1 = v1->to_expr_list ())) {
      /* noop, we're not dealing with lists! */
    } else if (!(v2 = _t2->eval_freeze (e)) || !(l2 = v2->to_expr_list ())) {
      report_error (e, "addition on lists takes two lists");
    } else if (!_pos) {
      report_error (e, "cannot subtract lists; can only add them");
    } else  {
      out = New refcounted<expr_list_t> (l1->lineno ());
      (*out) += *l1;
      (*out) += *l2;
    }
  }
  return out;
}

//-----------------------------------------------------------------------

scalar_obj_t 
pub3::expr_add_t::eval_internal (eval_t e) const
{
  scalar_obj_t out;

  if (_t1 && !_t1->eval_as_null (e) && _t2 && !_t2->eval_as_null (e)) {
    bool l = e.set_loud (true);
    scalar_obj_t o1 = _t1->eval_as_scalar (e);
    scalar_obj_t o2 = _t2->eval_as_scalar (e);
    e.set_loud (l);
    int64_t i1, i2;
    u_int64_t u1, u2;
    str s1, s2;
    int mul = _pos ? 1 : -1;
    if (o1.to_int64 (&i1) && o2.to_int64 (&i2)) {
      out.set_i (i1 + mul*i2);
    } else if (o1.to_uint64 (&u1) && o2.to_uint64 (&u2)) {
      if (mul > 0) {
	out.set_u (u1 + u2);
      } else {
	out.set_i (u1 + mul *u2);
      }
    } else if (o1.to_int64 (&i1) && o2.to_uint64 (&u2)) {
      out.set_i (i1 + mul*u2);
    } else if (o1.to_uint64 (&u1) && o2.to_int64 (&i2)) {
      out.set_i (u1 + mul*i2);
    } else if ((s1 = o1.to_str ()) && (s2 = o2.to_str ())) {
      strbuf b;
      b << s1 << s2;
      out.set (b);
    } else {
      report_error (e, "no plausible evaluation found!");
    }
  } else {
    report_error (e, "one or more operands were NULL");
  }

  return out;
}

//-----------------------------------------------------------------------

bool 
pub3::expr_arithmetic_t::eval_as_bool (eval_t e) const
{
  return eval_as_scalar (e).to_bool ();
}

//-----------------------------------------------------------------------

str 
pub3::expr_arithmetic_t::eval_as_str (eval_t e) const
{
  return eval_as_scalar (e).to_str ();
}

//-----------------------------------------------------------------------

str
pub3::expr_add_t::eval_as_str (eval_t e) const
{
  ptr<expr_list_t> l = eval_as_frozen_list (e);
  str s;
  if (l) {
    s = l->eval_as_str (e);
  } else {
    s = eval_internal (e).to_str ();
  }
  return s;
}

//-----------------------------------------------------------------------

int64_t 
pub3::expr_arithmetic_t::eval_as_int (eval_t e) const
{
  return eval_as_scalar (e).to_int64 ();
}

//-----------------------------------------------------------------------

u_int64_t
pub3::expr_arithmetic_t::eval_as_uint (eval_t e) const
{
  return eval_as_scalar (e).to_uint64 ();
}

//-----------------------------------------------------------------------

void
pub3::expr_shell_str_t::make_str (strbuf *b, vec<str> *v)
{
  if (b->tosuio ()->resid ()) {
    _els->push_back (New refcounted<pub3::expr_str_t> (*b));
    b->tosuio ()->clear ();
    v->setsize (0);
  }
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_shell_str_t::compact () const
{
  str s;
  ptr<expr_t> ret;

  if (_els->size () == 1 && (*_els)[0]->to_str ()) {
    ret = (*_els)[0];
  } else {
    ptr<pub3::expr_shell_str_t> out = 
      New refcounted<pub3::expr_shell_str_t> (_lineno);
    
    strbuf b;
    vec<str> v;
    
    for (size_t i = 0; i < _els->size (); i++) {
      ptr<expr_t> e = (*_els)[i];
      str s = e->to_str ();
      if (s) {
	b << s;
	v.push_back (s);
      } else {
	out->make_str (&b, &v);
	out->add (e);
      }
    }
    out->make_str (&b, &v);
    ret = out;
  }
   
  return ret;
}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_shell_str_t::eval_freeze (eval_t e) const
{
  str s = eval_internal (e);
  return New refcounted<expr_str_t> (s);
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_shell_str_t::eval_as_scalar (eval_t e) const
{
  return scalar_obj_t (eval_internal (e));
}

//-----------------------------------------------------------------------

str
pub3::expr_shell_str_t::eval_as_str (eval_t e) const
{
  str s = eval_internal (e);
  if (e.in_json ()) s = json::quote (s);
  return s;
}

//-----------------------------------------------------------------------

str
pub3::expr_shell_str_t::eval_internal (eval_t e) const
{
  bool err = false;
  DEBUG_ENTER ();
  if (_cache_generation != e.cache_generation ()) { 

    strbuf b;
    vec<str> v;
    for (size_t i = 0; !err && i < _els->size (); i++) {
      str s = (*_els)[i]->eval_as_str (e);

      size_t sz = b.tosuio ()->resid ();
      if (sz > size_t (max_shell_strlen)) {
	report_error (e, strbuf ("max-len string encountered (%zu)", sz));
	err = true;
      } else if (s) {
	b << s;
	v.push_back (s);
      }
    }

    _cache = b;
    _cache_generation = e.cache_generation ();
  }
  DEBUG_EXIT ("");
  return _cache;
}

//-----------------------------------------------------------------------

pub3::eval_t *
pub3::eval_t::link_to_penv ()
{
  eval_t *ret = penv ()->get_pub3_eval ();
  penv ()->set_pub3_eval (this);
  return ret;
}

//-----------------------------------------------------------------------

void
pub3::eval_t::unlink_from_penv (pub3::eval_t *e)
{
  penv ()->set_pub3_eval (e);
}

//-----------------------------------------------------------------------

ptr<slot_ref_t>
pub3::eval_t::lhs_resolve (const expr_t *e, const str &nm)
{
  ptr<slot_ref_t> ret;
  vec<const aarr_t *> &stk = _env->get_eval_stack ();

  if (_stack_p == ssize_t (EVAL_INIT)) {
    _stack_p = stk.size () - 1;
  }

  while (!ret && _stack_p >= 0) {
    ret = stk[_stack_p--]->const_cast_hack ()->lookup_slot (nm, false);
  }

  // If an abject failure, the assignment will allocate (implicitly)
  // a new GLOBAL variable.
  if (!ret) {
    aarr_t *odd; // output dict dest
    ptr<aarr_t> edd; // env dict dest

    if (_output && (odd = _output->dict_dest ())) {
      ret = odd->lookup_slot (nm);
    } else if ((edd = _env->get_global_aarr ())) {
      ret = edd->lookup_slot (nm);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::eval_t::resolve (const expr_t *e, const str &nm)
{
  ptr<const pval_t> ret;
  const vec<const aarr_t *> &stk = _env->get_eval_stack ();

  if (_stack_p == ssize_t (EVAL_INIT)) {
    _stack_p = stk.size () - 1;
  }

  // After every lookup, go down one frame in the stack, regardless
  // of whether we found what we were looking for or not.
  while (!ret && _stack_p >= 0) { 
    ret = stk[_stack_p--]->lookup_ptr (nm);
  }

  if (!ret && loud ()) {
    strbuf b ("cannot resolve variable: '%s'", nm.cstr ());
    e->report_error (*this, b);
  }

  return ret;
}

//-----------------------------------------------------------------------

void
pub3::expr_dict_t::add (binding_t p)
{
  (*this)[p.name ()] = p.expr ();
}

//-----------------------------------------------------------------------

void
pub3::inline_var_t::output (output_t *o, penv_t *e) const
{
  eval_t eval (e, o);
  str s;
  if (!_expr) { 
    /* noop */
  } else if ((s = _expr->eval_as_str (eval))) {
    o->output (e, s, false);
  }
}

//-----------------------------------------------------------------------

void
pub3::inline_var_t::dump2 (dumper_t *d) const
{
  DUMP(d, "pub3::inline_var_t (line=" << _lineno << ")");
}

//-----------------------------------------------------------------------

pub3::eval_t::eval_t (penv_t *e, output_t *o)
  : _env (e), 
    _output (o), 
    _loud (false), 
    _silent (false), 
    _stack_p (EVAL_INIT),
    _in_json (false) {}

//-----------------------------------------------------------------------

pub3::eval_t::~eval_t () {}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_dict_t::eval_freeze (eval_t e) const
{
  ptr<expr_dict_t> ret = New refcounted<expr_dict_t> (_lineno);
  if (_dict) {
    e.eval_freeze_dict (_dict, ret->dict ());
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_list_t::eval_freeze (eval_t e) const
{
  ptr<expr_list_t> ret = New refcounted<expr_list_t> (_lineno);
  e.eval_freeze_vec (this, ret);
  return ret;
}

//-----------------------------------------------------------------------

void
pub3::eval_t::eval_freeze_vec (const vec_iface_t *in, vec_iface_t *out)
{
  ptr<pval_t> v;
  ptr<expr_t> e;
  for (size_t i = 0; i < in->size (); i++) {
    v = eval_freeze (in->lookup (i));
    out->push_back (v);
  }
}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::eval_t::eval_freeze (ptr<const pval_t> in)
{
  ptr<pval_t> out;
  ptr<const expr_t> e;
  if (!in) { 
    /* noop */
  } else if ((e = in->to_expr ())) {
    out = e->eval_freeze (*this);
  } else {
    out = in->copy_stub ();
  }
  return out;
}

//-----------------------------------------------------------------------

void
pub3::eval_t::eval_freeze_dict (const aarr_t *in, aarr_t *out)
{
  ptr<pval_t> val, v;
  ptr<expr_t> e;
  const nvtab_t *nvt = in->nvtab ();

  vec<binding_t *> additions;
  vec<str> removals;

  // To make this deterministic, we must not make any changes
  // to out as we go through the table.  Otherwise, the behavior
  // changes based on the sort order of the hash table, which isn't
  // pretty.  Just do all changes at the end, after all evaluations
  // have completed....
  for (binding_t *p = nvt->first (); p; p = nvt->next (p)) {
    val = eval_freeze (p->value_ptr ());
    if (val) {
      additions.push_back (New nvpair_t (p->name (), val));
    } else {
      removals.push_back (p->name ());
    }
  }

  for (size_t i = 0; i < additions.size (); i++) {
    out->add (additions[i]);
  }

  for (size_t i = 0; i < removals.size (); i++) {
    out->remove (removals[i]);
  }
}

//-----------------------------------------------------------------------

pub3::expr_shell_str_t::expr_shell_str_t (int lineno)
  : expr_t (lineno), 
    _els (expr_list_t::alloc (lineno))  {}

//-----------------------------------------------------------------------

pub3::expr_shell_str_t::expr_shell_str_t (const str &s, int lineno)
  : expr_t (lineno), 
    _els (expr_list_t::alloc (lineno)) 
{ _els->push_back (New refcounted<expr_str_t> (s)); }

//-----------------------------------------------------------------------

pub3::expr_shell_str_t::expr_shell_str_t (ptr<expr_t> e, int lineno)
  : expr_t (lineno),
    _els (expr_list_t::alloc (lineno)) 
{ _els->push_back (e); }

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_list_t::lookup (ssize_t s, bool *ibp) const
{
  ptr<const pval_t> r;
  bool ib;
  if ((ib = fixup_index (&s))) {
    r = (*this)[s];
  }

  if (ibp) *ibp = ib;
  return r;
}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_list_t::lookup (ssize_t s, bool *ibp)
{
  ptr<pval_t> r;
  bool ib;

  if ((ib = fixup_index (&s))) {
    r = (*this)[s];
  }

  if (ibp) *ibp = ib;
  return r;
}

//-----------------------------------------------------------------------

void
pub3::expr_list_t::set (size_t i, ptr<pval_t> v)
{
  ptr<expr_t> e = v->to_expr ();
  if (e) {
    if (i >= size ())
      setsize (i + 1);
    (*this)[i] = e;
  }
}

//-----------------------------------------------------------------------

void
pub3::expr_list_t::push_back (ptr<pval_t> v)
{
  ptr<expr_t> e;
  if (v) { e = v->to_expr (); }
  if (!e) { e = expr_null_t::alloc (); }
  vec_base_t::push_back (e);
}

//-----------------------------------------------------------------------

static str
vec2str (const vec<str> &v, char o, char c)
{
  strbuf b ("%c", o);
  for (size_t i = 0; i < v.size (); i++) {
    if (i != 0) b << ", ";
    b << v[i];
  }
  b.fmt ("%c", c);
  return b;
}

//-----------------------------------------------------------------------

str
pub3::expr_list_t::eval_as_str (eval_t e) const
{
  DEBUG_ENTER ();
  vec<str> v;
  e.set_in_json ();
  for (size_t i = 0; i < size (); i++) {
    ptr<const expr_t> x = (*this)[i];
    str s;
    if (x) { s = x->eval_as_str (e); }
    v.push_back (json::safestr (s));
  }

  str ret = vec2str (v, '[', ']');
  DEBUG_EXIT (ret);
  return ret;
}

//-----------------------------------------------------------------------

str
pub3::expr_list_t::to_str () const
{
  DEBUG_ENTER ();
  vec<str> v;

  for (size_t i = 0; i < size (); i++) {
    ptr<const expr_t> x = (*this)[i];
    str s;
    if (x) { s = x->to_str (); }
    v.push_back (json::safestr (s));
  }

  str ret = vec2str (v, '[', ']');
  DEBUG_EXIT (ret);
  return ret;
}

//-----------------------------------------------------------------------

str
pub3::expr_dict_t::eval_as_str (eval_t e) const
{
  vec<str> v;
  DEBUG_ENTER ();

  e.set_in_json ();

  if (_dict) {
    const nvtab_t *nvt = _dict->nvtab ();
    for (const binding_t *p = nvt->first (); p; p = nvt->next (p)) {
      str valstr;
      ptr<const pval_t> pv = p->value_ptr ();
      ptr<const expr_t> x;
      ptr<const pub_scalar_t> ps;

      if (!pv) {
	/* noop */ 
      } else if ((x = pv->to_expr ())) {
	valstr = x->eval_as_str (e);
      } else if ((ps = pv->to_pub_scalar ())) {
	valstr = ps->obj ().to_str ();
      }
      valstr = json::safestr (valstr);
      str namestr = json::quote (p->name ());
      strbuf b ("%s : %s", namestr.cstr (), valstr.cstr ());
      v.push_back (b);
    }
  }
  str ret = vec2str (v, '{', '}');
  DEBUG_EXIT (ret);
  return ret;
}


//-----------------------------------------------------------------------

str
pub3::expr_dict_t::to_str () const
{
  vec<str> v;
  DEBUG_ENTER ();
  if (_dict) {
    const nvtab_t *nvt = _dict->nvtab ();
    for (const nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
      str valstr;
      ptr<const pval_t> pv = p->value_ptr ();
      ptr<const expr_t> x;
      ptr<const pub_scalar_t> ps;
      if (!pv) {
	/* noop */ 
      } else if ((x = pv->to_expr ())) {
	valstr = x->to_str ();
      } else if ((ps = pv->to_pub_scalar ())) {
	valstr = ps->obj ().to_str ();
      }
      valstr = json::safestr (valstr);
      strbuf b ("%s : %s", p->name ().cstr (), valstr.cstr ());
      v.push_back (b);
    }
  }
  str ret = vec2str (v, '{', '}');
  DEBUG_EXIT (ret);
  return ret;
}

//-----------------------------------------------------------------------

scalar_obj_t 
pub3::expr_list_t::to_scalar () const
{
  DEBUG_ENTER ();
  scalar_obj_t so = scalar_obj_t (to_str ());
  DEBUG_EXIT ("");
  return so;
}

//-----------------------------------------------------------------------

scalar_obj_t 
pub3::expr_dict_t::to_scalar () const
{
  DEBUG_ENTER ();
  scalar_obj_t so = scalar_obj_t (to_str ());
  DEBUG_EXIT ("");
  return so;
}

//-----------------------------------------------------------------------

int64_t
pub3::expr_uint_t::to_int () const
{
  int64_t out = 0;
  if (_val <= u_int64_t (INT64_MAX)) {
    out = _val;
  }
  return out;
}

//-----------------------------------------------------------------------

bool
pub3::expr_uint_t::to_int64 (int64_t *out) const
{
  bool ret = false;
  if (_val <= u_int64_t (INT64_MAX)) {
    ret = true;
    *out = _val;
  }
  return ret;
}


//-----------------------------------------------------------------------

u_int64_t
pub3::expr_int_t::to_uint () const
{
  u_int64_t out = 0;
  to_uint (&out);
  return out;
}

//-----------------------------------------------------------------------

bool
pub3::expr_int_t::to_uint (u_int64_t *u) const
{
  bool r = false;
  if (_val >= 0) {
    *u = _val;
    r = true;
  }
  return r;
}

//-----------------------------------------------------------------------
    
//=======================================================================
// Shortcuts

ptr<const aarr_t>
pub3::expr_t::eval_as_dict (eval_t e) const
{
  ptr<const pval_t> v;
  ptr<const expr_t> x;
  ptr<const aarr_t> ret;

  if (!(v = eval (e))) {
    /* noop */
  } else if ((x = v->to_expr ())) {
    ret = x->to_dict ();
  } else {
    ret = v->to_aarr ();
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<const vec_iface_t>
pub3::expr_t::eval_as_vec (eval_t e) const
{
  ptr<const pval_t> v = eval (e);
  ptr<const vec_iface_t> ret;
  if (v) ret = v->to_vec_iface ();
  return ret;
}

//-----------------------------------------------------------------------

bool 
pub3::expr_t::eval_as_vec_or_dict (eval_t e, ptr<const vec_iface_t> *vp, 
				   ptr<const aarr_t> *dp) const
{
  ptr<const pval_t> v;
  ptr<const expr_t> x;

  bool ret = false;

  if (!(v = eval (e))) {
    ret = false;
  } else if ((*vp = v->to_vec_iface ())) {
    ret = true;
  } else if ((x = v->to_expr ()) && (*dp = x->to_dict ())) {
    ret = true; 
  } else if ((*dp = v->to_aarr ())) {
    ret = true;
  }

  return ret;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_t::eval_as_scalar (eval_t e) const
{
  ptr<const pval_t> v;
  ptr<const expr_t> x;
  ptr<const pub_scalar_t> ps;
  scalar_obj_t ret;

  DEBUG_ENTER();
  v = eval_freeze (e);

  // For pub v3 objects, they should resolve to an expression...
  if (v && (x = v->to_expr ())) {
    ret = x->to_scalar ();

    // Some pub v1 or v2 objects are actually wrapped scalars.
  } else if (v && (ps = v->to_pub_scalar ())) {
    ret = ps->obj ();

    // And generic pub v1 and v2 are evaluable to scalars...
  } else if (v && v->eval_to_scalar (e.penv (), &ret)) {
    /* noop; all good! */

  } else if (e.loud ()) {
    strbuf b ("cannot convert to scalar");
    report_error (e, b);
  }

  DEBUG_EXIT ("");
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_t::eval_as_bool (eval_t e) const
{
  scalar_obj_t so = eval_as_scalar (e);
  return so.to_bool ();
}

//-----------------------------------------------------------------------

int64_t
pub3::expr_t::eval_as_int (eval_t e) const
{
  scalar_obj_t so = eval_as_scalar (e);
  int64_t r = 0;

  if (!so.to_int64 (&r) && e.loud ()) {
    str tmp = so.to_str ();
    if (!tmp) tmp = "<none>";
    strbuf b ("cannot convert '%s' to int64", tmp.cstr ());
    report_error (e, b);
  }

  return r;
}

//-----------------------------------------------------------------------

u_int64_t
pub3::expr_t::eval_as_uint (eval_t e) const
{
  scalar_obj_t so = eval_as_scalar (e);
  u_int64_t r = 0;

  if (!so.to_uint64 (&r) && e.loud ()) {
    str tmp = so.to_str ();
    if (!tmp) tmp = "<none>";
    strbuf b ("cannot convert '%s' to uint64", tmp.cstr ());
    report_error (e, b);
  }

  return r;
}

//-----------------------------------------------------------------------

str
pub3::expr_t::eval_as_str (eval_t e) const
{
  str r;
  ptr<const pval_t> v;
  ptr<const expr_t> x;

  if (!(v = eval_freeze (e))) {
    /* noop */
  } else if ((x = v->to_expr ())) {
    r = x->to_str ();
  } else {
    // shouldn't get here!
    r = eval_as_scalar (e).to_str ();
  }
  return r;
}

//-----------------------------------------------------------------------

str 
pub3::expr_str_t::eval_as_str (eval_t e) const
{
  str ret;
  ret = to_str ();
  if (e.in_json ()) ret = json::quote (ret);
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::to_len (size_t *s) const
{
  *s = _val ? _val.len () : 0;
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::to_len (size_t *s) const
{
  *s = size ();
  return true;
}

//-----------------------------------------------------------------------

bool
pub3::expr_dict_t::to_len (size_t *s) const 
{
  *s = _dict ? _dict->size () : 0;
  return true;
}

//=======================================================================

// Pub v1/v2

void
pub3::expr_t::eval_obj (pbuf_t *b, penv_t *e, u_int d) const
{
  eval_t ev (e, NULL);
  eval_t *evp = e->get_pub3_eval ();

  if (!evp) evp = &ev;

  ptr<const pval_t> pv = eval (*evp);
  ptr<const expr_t> x;

  if (pv && (x = pv->to_expr ())) {
    str s = x->to_str ();
    if (s) b->add (s);
  } else if (pv) {

    // v1 and v2 objects; note need to think about corner cases
    // with infinite recursion.  Make sure we pass through our pub
    // state is the way to solve the issue...
    eval_t *old = evp->link_to_penv ();
    pv->eval_obj (b, e, d);
    evp->unlink_from_penv (old);

  } else if (e->debug ()) {
    e->setlineno (_lineno);
    str nm = to_identifier ();
    if (!nm) {
      nm = "-- unknown --";
    }
    e->warning (strbuf ("cannot resolve variable: " ) << nm.cstr ());
    b->add (strbuf ("<!--UNDEF: ") << nm << " -->");
    e->unsetlineno ();
  }
}

//-----------------------------------------------------------------------

ptr<pub_scalar_t>
pub3::expr_t::to_pub_scalar ()
{
  return New refcounted<pub_scalar_t> (to_scalar ());
}

//-----------------------------------------------------------------------

ptr<const pub_scalar_t>
pub3::expr_t::to_pub_scalar () const
{
  return New refcounted<pub_scalar_t> (to_scalar ());
}

//=======================================================================
// regex's

//-----------------------------------------------------------------------

static pub3::rxx_factory_t g_rxx_factory;

//-----------------------------------------------------------------------

ptr<rxx>
pub3::rxx_factory_t::compile (str body, str opts, str *errp)
{
  return g_rxx_factory._compile (body, opts, errp);
}

//-----------------------------------------------------------------------

ptr<rxx>
pub3::rxx_factory_t::_compile (str body, str opts, str *errp)
{
  const char *b = body;
  const char *o = "";
  if (opts) o = opts;

  ptr<rxx> *rp;
  ptr<rxx> ret;

  strbuf k ("%s-%s", b, o);
  if ((rp = _cache[k])) { 
    ret = *rp; 
  } else {
    ptr<rrxx> tmp = New refcounted<rrxx> ();
    if (!tmp->compile (b, o)) {
      strbuf b;
      str err = tmp->geterr ();
      b << "Cannot compile regex '" << b << "' with options '"
	<< o << "': " << err << "\n";
      if (errp) *errp = b;
    }
    _cache.insert (k, tmp);
    ret = tmp;
  }
  return ret;
}

//-----------------------------------------------------------------------

pub3::expr_regex_t::expr_regex_t (int lineno) : expr_t (lineno) {}

//-----------------------------------------------------------------------

pub3::expr_regex_t::expr_regex_t (ptr<rxx> x, str b, str o, int l)
  : expr_t (l), _rxx (x), _body (b), _opts (o) {}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_regex_t::eval_freeze (eval_t e) const
{
  return copy_stub ();
}

//-----------------------------------------------------------------------

ptr<rxx>
pub3::expr_list_t::eval_as_regex (eval_t e) const
{
  str opts;
  str body;
  ptr<rxx> ret;

  if (size () >= 2) {
    opts = (*this)[1]->eval_as_str (e);
  }
  
  if (size () >= 1) {
    body = (*this)[0]->eval_as_str (e);
  }
  
  ret = str2rxx (&e, body, opts);

  return ret;
}

//-----------------------------------------------------------------------

ptr<rxx>
pub3::expr_t::str2rxx (const eval_t *e, const str &b, const str &o) const
{
  ptr<rxx> ret;
  if (b) {
    str err;
    ret = rxx_factory_t::compile (b, o, &err);
    if (e && e->loud () && err) {
      report_error (*e, err);
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<rxx>
pub3::expr_shell_str_t::eval_as_regex (eval_t e) const
{
  ptr<rxx> ret;
  str s = eval_as_str (e);
  ret = str2rxx (&e, s, NULL);
  return ret;
}

//-----------------------------------------------------------------------

ptr<rxx>
pub3::expr_str_t::to_regex () const
{
  ptr<rxx> ret;
  ret = str2rxx (NULL, _val, NULL);
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_varref_or_rfn_t::unshift_argument (ptr<expr_t> x)
{
  if (!_arglist) {
    _arglist = New refcounted<expr_list_t> (_lineno);
  }
  _arglist->push_front (x);
  return true;
}

//-----------------------------------------------------------------------

#define EXPR_VARREF_EVAL(ret,func) \
  ret							\
  pub3::expr_varref_or_rfn_t::func (eval_t e) const	\
  {							\
    ret r;						\
    ptr<const expr_t> rfn = get_rfn ();			\
    if (rfn) {						\
      r = rfn->func (e);				\
    } else {						\
      r = expr_varref_t::func (e);			\
    }							\
    return r;						\
  }

EXPR_VARREF_EVAL(ptr<const pval_t>, eval)
EXPR_VARREF_EVAL(ptr<pval_t>, eval_freeze)
EXPR_VARREF_EVAL(str, eval_as_str)
EXPR_VARREF_EVAL(ptr<rxx>, eval_as_regex)
EXPR_VARREF_EVAL(bool, eval_as_null)

//-----------------------------------------------------------------------

ptr<const pub3::expr_t>
pub3::expr_varref_or_rfn_t::get_rfn () const
{
  if (_arglist && !_rfn) {
    _rfn = pub3::rfn_factory_t::get ()->alloc (_name, _arglist, _lineno);
  }
  return _rfn;
}

//-----------------------------------------------------------------------

void
pub3::expr_list_t::push_front (ptr<expr_t> e)
{
  // XXX - hack -- push_back and then bubble to the front!
  push_back (e);
  for (size_t i = size () - 1; i > 0; i--) {
    ptr<expr_t> tmp = (*this)[i];
    (*this)[i] = (*this)[i-1];
    (*this)[i-1] = tmp;
  }
}

//-----------------------------------------------------------------------

ptr<pub3::expr_dict_t>
pub3::expr_dict_t::copy_stub_dict () const
{
  return mkref (const_cast<expr_dict_t *> (this));
}

//-----------------------------------------------------------------------

void
pub3::expr_dict_t::replace (const str &nm, ptr<expr_t> x)
{
  dict ()->replace (nm, x);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_str_t>
pub3::expr_str_t::alloc (const str &s)
{
  return New refcounted<expr_str_t> (s);
}

//-----------------------------------------------------------------------

ptr<slot_ref_t>
pub3::expr_list_t::lookup_slot (ssize_t i)
{
  ptr<slot_ref_t> ret;
  ssize_t ssz = size ();

  fixup_index (&i, true);

  if (i >= ssz) {
    setsize (i+1);
  }
  ret = slot_ref3_t::alloc (&(*this)[i]);
  return ret;
}

//-----------------------------------------------------------------------

void
pub3::slot_ref3_t::set_expr (ptr<pub3::expr_t> e)
{
  assert (_epp);
  *_epp = e;
}

//-----------------------------------------------------------------------

void
pub3::slot_ref3_t::set_pval (ptr<pval_t> p)
{
  assert (_epp);
  ptr<expr_t> x;
  if (p) { x = p->to_expr (); }
  *_epp = x;
}

//-----------------------------------------------------------------------

ptr<pval_t> 
pub3::slot_ref3_t::deref_pval () const
{
  assert (_epp);
  return *_epp;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::slot_ref3_t::deref_expr () const
{
  assert (_epp);
  return *_epp;
}

//-----------------------------------------------------------------------

pub3::expr_assignment_t::expr_assignment_t (ptr<pub3::expr_t> lhs,
					    ptr<pub3::expr_t> rhs,
					    int lineno)
  : _lhs (lhs), _rhs (rhs), _lineno (lineno) {}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_assignment_t::eval_internal (eval_t e) const
{
  ptr<const pval_t> rhs;
  ptr<pval_t> mrhs;
  ptr<slot_ref_t> slot;
  ptr<expr_ref_t> rf;
  ptr<const expr_t> x;

  if (_rhs) { mrhs = _rhs->eval_freeze (e); }

  assert (_lhs);
  ptr<expr_t> mlhs = _lhs->const_cast_hack ();

  if (!(rf = mlhs->to_ref ())) {
    report_error (e, "left-hand side of assignment was not a reference");
  } else if (!(slot = rf->lhs_deref (&e))) {
    report_error (e, "left-hand side of assignment was not settable");
  } else {
    slot->set_pval (mrhs);
  }

  return mrhs;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_t>
pub3::expr_t::const_cast_hack () const
{
  return mkref (const_cast<expr_t *> (this));
}

//-----------------------------------------------------------------------

bool
pub3::expr_list_t::fixup_index (ssize_t *ip, bool lax) const
{
  bool ret = false;
  ssize_t i = *ip;
  ssize_t sz = size ();

  if (i >= 0 && i < sz) {
    ret = true; 
  } else {

    // apply from-back-indexing.
    if (i < 0) {
      i += sz;
    }

    if (i < 0) {
      ret = lax;
      if (lax) i = 0;
    } else if (i >= sz) {
      ret = lax;
    } else {
      ret = true;
    }

  }
  *ip = i;
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_bool_t> pub3::expr_bool_t::_false;
ptr<pub3::expr_bool_t> pub3::expr_bool_t::_true;

ptr<pub3::expr_bool_t>
pub3::expr_bool_t::alloc (bool b)
{
  ptr<expr_bool_t> &retref = b ? _true : _false;
  if (!retref) { retref = New refcounted<expr_bool_t> (b); }
  return retref;
}

//-----------------------------------------------------------------------

namespace pub3 {

  //-----------------------------------------------------------------------

  ptr<pair_t> pair_t::alloc (const str &k, ptr<expr_t> x)
  { return New refcounted<pair_t> (k, x); }

  //-----------------------------------------------------------------------

  void bindlist_t::add (binding_t b) { push_back (b); }
  
  //-----------------------------------------------------------------------

  void
  bindtab_t::overwrite_with (const bindtab_t &t)
  {
    qhash_const_iterator_t<str, ptr<expr_t> > it (t);
    str k;
    ptr<expr_t> v;
    while ((k = it.next (&v))) { insert (k, v); }
  }

  //-----------------------------------------------------------------------

  bindtab_t &
  bindtab_t::operator+= (const bindtab_t &t)
  {
    overwrite_with (b);
    return *this;
  }

  //-----------------------------------------------------------------------

};

//=======================================================================


