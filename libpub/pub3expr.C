
#include "pub3expr.h"
#include "parseopt.h"
#include "okformat.h"

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_t::flatten (eval_t e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar (e));
}

//-----------------------------------------------------------------------

bool
pub3::expr_OR_t::eval_as_bool (eval_t e) const
{
  bool ret = false;
  ret = ((_t1 && _t1->eval_as_bool (e)) || (_t2 && _t2->eval_as_bool (e)));
  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_AND_t::eval_as_bool (eval_t e) const
{
  bool ret;
  ret =  ((_f1 && _f1->eval_as_bool (e)) && (_f2 && _f2->eval_as_bool (e)));
  return ret;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_logical_t::eval_as_scalar (eval_t e) const
{
  scalar_obj_t o;
  o.set (eval_as_int (e));
  return o;
}

//-----------------------------------------------------------------------

str
pub3::expr_logical_t::eval_as_str (eval_t e) const
{
  bool b = eval_as_bool (e);
  return b ? "True" : "False";
}

//-----------------------------------------------------------------------

bool
pub3::expr_EQ_t::eval_as_bool (eval_t e) const
{
  bool ret = false;

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
  ret = (tmp ^ flip);

  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_relation_t::eval_as_bool (eval_t e) const
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
pub3::expr_NOT_t::eval_as_bool (eval_t e) const
{
  bool ret = true;
  if (_e) ret = !(_e->eval_as_bool (e));
  return ret;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_dictref_t::eval_as_pval (eval_t e) const
{
  ptr<const aarr_t> d;
  ptr<const pval_t> v;
  if (!_dict) {
    report_error (e, "dict reference into NULL");
  } else if (!(d = _dict->eval_as_dict (e))) {
    report_error (e, "dict reference into non-dict");
  } else if (!(v = d->lookup_ptr (_key))) {
    strbuf b ("cannot resolve key '%s'", _key.cstr ());
    report_error (e, b);
  }
  return v;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_vecref_t::eval_as_pval (eval_t e) const
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
pub3::expr_t::eval_as_scalar (eval_t e) const
{
  if (_cache_generation != e.cache_generation ()) {
    ptr<const pval_t> v = eval_as_pval (e);
    ptr<const pub_scalar_t> s;
    if (v && (s = v->to_scalar ())) {
      _so = s->obj ();
    }
    _cache_generation = e.cache_generation ();
  }
  return _so;
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
  scalar_obj_t so = eval_as_scalar (e);
  return so.to_str ();
}

//-----------------------------------------------------------------------

bool
pub3::expr_t::is_null (eval_t e) const
{
  return eval_as_pval (e) == NULL;
}

//-----------------------------------------------------------------------

ptr<const aarr_t>
pub3::expr_t::eval_as_dict (eval_t e) const
{
  ptr<const aarr_arg_t> r;
  ptr<const pval_t> v;
  if ((v = eval_as_pval (e)))  {
    r = v->to_aarr ();
  }
  return r;
}

//-----------------------------------------------------------------------

ptr<const parr_mixed_t>
pub3::expr_t::eval_as_vec (eval_t e) const
{
  ptr<const parr_mixed_t> r;
  ptr<const pval_t> v;

  if ((v = eval_as_pval (e)))  {
    r = v->to_mixed_arr ();
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
pub3::expr_varref_t::eval_as_pval (eval_t e) const
{
  ptr<const pval_t> ret;
  ret = e.resolve (this, _name);
  return ret;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_varref_t::eval_as_scalar (eval_t e) const
{
  ptr<const pval_t> v;
  ptr<const expr_t> x;
  ptr<const pub_scalar_t> ps;
  scalar_obj_t ret;

  v = e.resolve (this, _name);

  // For pub v3 objects, they should resolve to an expression...
  if (v && (x = v->to_expr ())) {
    ret = x->eval_as_scalar (e);


    // Some pub v1 or v2 objects are actually wrapped scalars.
  } else if (v && (ps = v->to_scalar ())) {
    ret = ps->obj ();

    // And generic pub v1 and v2 are evaluable to scalars...
  } else if (v && v->eval_to_scalar (e.penv (), &ret)) {
    /* noop; all good! */

  } else if (e.loud ()) {
    strbuf b ("cannot resolve: %s", _name.cstr ());
    report_error (e, b);
  }

  return ret;
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::eval_as_bool (eval_t e) const
{
  return (_val && _val.len ());
}

//-----------------------------------------------------------------------

int64_t
pub3::expr_str_t::eval_as_int (eval_t e) const
{
  int64_t ret = 0;
  if (_val)
    convertint (_val, &ret);
  return ret;
}

//-----------------------------------------------------------------------

u_int64_t
pub3::expr_str_t::eval_as_uint (eval_t e) const
{
  u_int64_t ret = 0;
  if (_val)
    convertuint (_val, &ret);
  return ret;
}

//-----------------------------------------------------------------------

str
pub3::expr_str_t::eval_as_str (eval_t e) const
{
  return _val;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_str_t::eval_as_scalar (eval_t e) const
{
  return scalar_obj_t (_val);
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_str_t::eval_as_pval (eval_t e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar (e));
}

//-----------------------------------------------------------------------

bool
pub3::expr_str_t::is_null (eval_t e) const
{
  return !_val;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_int_t::eval_as_scalar (eval_t e) const
{
  scalar_obj_t so;
  so.set (_val);
  return so;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_double_t::eval_as_scalar (eval_t e) const
{
  scalar_obj_t so;
  so.set (_val);
  return so;
}

//-----------------------------------------------------------------------

scalar_obj_t
pub3::expr_uint_t::eval_as_scalar (eval_t e) const
{
  scalar_obj_t so;
  so.set_u (_val);
  return so;
}

//-----------------------------------------------------------------------

ptr<const pval_t>
pub3::expr_number_t::eval_as_pval (eval_t e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar(e)); 
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
    _so = eval_as_scalar_nocache (e);
    _cache_generation = e.cache_generation ();
  }
  return _so;
}


//-----------------------------------------------------------------------

scalar_obj_t 
pub3::expr_add_t::eval_as_scalar_nocache (eval_t e) const
{
  scalar_obj_t out;

  if (_t1 && !_t1->is_null (e) && _t2 && !_t2->is_null (e)) {
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
pub3::expr_arithmetic_t:: eval_as_uint (eval_t e) const
{
  return eval_as_scalar (e).to_uint64 ();
}

//-----------------------------------------------------------------------

ptr<const pval_t> 
pub3::expr_arithmetic_t::eval_as_pval (eval_t e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar (e));
}

//-----------------------------------------------------------------------

ptr<const pval_t> 
pub3::expr_shell_str_t::eval_as_pval (eval_t e) const
{
  return New refcounted<pub_scalar_t> (eval_as_scalar (e));
}

//-----------------------------------------------------------------------

void
pub3::expr_shell_str_t::make_str (strbuf *b, vec<str> *v)
{
  if (b->tosuio ()->resid ()) {
    _els.push_back (New refcounted<pub3::expr_str_t> (*b));
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
  

  if (_els.size () == 1 && _els[0]->eval_as_str ()) {
    ret = _els[0];
  } else {
    ptr<pub3::expr_shell_str_t> out = 
      New refcounted<pub3::expr_shell_str_t> (_lineno);
    
    strbuf b;
    vec<str> v;
    
    for (size_t i = 0; i < _els.size (); i++) {
      ptr<expr_t> e = _els[i];
      str s = e->eval_as_str ();
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

scalar_obj_t
pub3::expr_shell_str_t::eval_as_scalar (eval_t e) const
{
  bool err = false;
  if (_cache_generation != e.cache_generation ()) { 

    strbuf b;
    vec<str> v;
    for (size_t i = 0; !err && i < _els.size (); i++) {
      str s = _els[i]->eval_as_str (e);

      size_t sz = b.tosuio ()->resid ();
      if (sz > size_t (max_shell_strlen)) {
	report_error (e, strbuf ("max-len string encountered (%zu)", sz));
	err = true;
      } else if (s) {
	b << s;
	v.push_back (s);
      }
    }

    _so.set (b);
    _cache_generation = e.cache_generation ();
  }
  return _so;
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
  } else if (ret && (xref = ret->to_ref ())) {
    ret = xref->deref (*this); // mutual recursive call!
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
  if (_expr){
    s = _expr->eval_as_str (eval);
  }
  if (s) {
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

void
pub3::expr_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const
{
  eval_t eval (e, NULL);
  ptr<const pval_t> pv = eval_as_pval (eval);
  if (pv) {
    pv->eval_obj (ps, e, d);
  } else if (e->debug ()) {
    e->setlineno (_lineno);
    str nm = eval_as_identifier ();
    if (!nm) {
      nm = "-- unknown --";
    }
    e->warning (strbuf ("cannot resolve variable: " ) << nm.cstr ());
    ps->add (strbuf ("<!--UNDEF: ") << nm << " -->");
    e->unsetlineno ();
  }
}

//-----------------------------------------------------------------------

pub3::eval_t::eval_t (penv_t *e, output_t *o)
  : _env (e), 
    _output (o), 
    _loud (false), 
    _silent (false), 
    _stack_p (EVAL_INIT) 
{ 
  e->bump (); 
}

//-----------------------------------------------------------------------

ptr<pval_t>
pub3::expr_dict_t::flatten (eval_t e) const
{
  ptr<aarr_arg_t> ret = New refcounted<aarr_arg_t> ();
  if (_dict) {
    e.flatten_dict (_dict, ret);
  }
  return ret;
}

//-----------------------------------------------------------------------

void
pub3::eval_t::flatten_dict (const aarr_t *in, aarr_t *out)
{
  ptr<pval_t> val, v;
  ptr<expr_t> e;
  const nvtab_t *nvt = in->nvtab ();
  for (nvpair_t *p = nvt->first (); p; p = nvt->next (p)) {
    val = p->value_ptr ();
    if (val && (e = val->to_expr ()) && (v = e->flatten (*this))) {
      val = v;
    }
    out->add (New nvpair_t (p->name (), val));
  }
}


//-----------------------------------------------------------------------
