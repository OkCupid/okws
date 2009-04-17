
#include "pub3expr.h"
#include "parseopt.h"
#include "okformat.h"
#include "pub3func.h"
#include "pescape.h"

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
pub3::expr_frozen_t::eval_freeze (eval_t e) const
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
    _cached_val = New refcounted<expr_bool_t> (_cached_bool);
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
    // use pub v1/v2 eval mechanism
    ret = v->eval (e.penv (), EVAL_INTERNAL, true);
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
    if (e->loud ()) report_error (*e, "vector is undefined");
  } else if (!_vec->eval_as_vec_or_dict (*e, &vif, &dict)) {
    report_error (*e, "vector reference into non-vector");
  } else if (vif) {
    size_t i = 0;
    if (_index) 
      i = _index->eval_as_int (*e);
    if (!(r = vif->lookup (i, &in_bounds)) && !in_bounds && e->loud ()) {
      report_error (*e, strbuf ("vector reference (%zd) out of bounds", i));
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
  scalar_obj_t so = eval_as_scalar (e);
  return expr_t::alloc (so);
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
    ret = New refcounted<expr_int_t> (i);
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
    report_error (e, "one or more operands were NULL");
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

ptr<const pval_t>
pub3::eval_t::resolve (const expr_t *e, const str &nm)
{
  ptr<const pval_t> ret, n;
  const vec<const aarr_t *> &stk = _env->get_eval_stack ();

  if (_stack_p == ssize_t (EVAL_INIT)) {
    _stack_p = stk.size () - 1;
  }

  // After every lookup, go down one frame in the stack, regardless
  // of whether we found what we were looking for or not.
  while (!ret && _stack_p >= 0) { 
    ret = stk[_stack_p--]->lookup_ptr (nm);
  }

  ptr<const expr_ref_t> xref;

  if (!ret && loud ()) {
    strbuf b ("cannot resolve variable: '%s'", nm.cstr ());
    e->report_error (*this, b);
  }

  return ret;
}

//-----------------------------------------------------------------------

void
pub3::expr_dict_t::add (nvpair_t *p)
{
  _dict->add (p);
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
    _in_json (false)
{ 
  e->bump (); 
}

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

  vec<nvpair_t *> additions;
  vec<str> removals;

  // To make this deterministic, we must not make any changes
  // to out as we go through the table.  Otherwise, the behavior
  // changes based on the sort order of the hash table, which isn't
  // pretty.  Just do all changes at the end, after all evaluations
  // have completed....
  for (nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
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
  if (s < 0 || s >= ssize_t (size ())) {
    ib = false;
  } else {
    ib = true;
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
  if (s < 0 || s >= ssize_t (size ())) {
    ib = false;
  } else {
    ib = true;
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
  ptr<expr_t> e = v->to_expr ();
  if (e) {
    vec_base_t::push_back (e);
  }
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
    for (const nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
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
      strbuf b ("%s : %s", p->name ().cstr (), valstr.cstr ());
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
  if (_val >= 0) {
    out = _val;
  }
  return out;
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
  ptr<const pval_t> pv = eval (ev);
  ptr<const expr_t> x;

  if (pv && (x = pv->to_expr ())) {
    str s = x->to_str ();
    if (s) b->add (s);
  } else if (pv) {
    // v1 and v2 objects; note need to think about corner cases
    // with infinite recursion.
    pv->eval_obj (b, e, d);
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

void
pub3::pstr_el_t::eval_obj (pbuf_t *s, penv_t *e, u_int d) const
{
  if (_expr) {
    _expr->eval_obj (s, e, d);
  }
}

//-----------------------------------------------------------------------

void
pub3::pstr_el_t::dump2 (dumper_t *d) const
{

}


//-----------------------------------------------------------------------

pfile_el_t *
pub3::pstr_el_t::to_pfile_el ()
{
  return New inline_var_t (_expr, -1);
}

//-----------------------------------------------------------------------

void 
pub3::pstr_el_t::output (output_t *o, penv_t *e) const 
{
  assert (false);
}

//=======================================================================


