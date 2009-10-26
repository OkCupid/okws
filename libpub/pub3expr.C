#include "okformat.h"
#include "pub3expr.h"
#include "parseopt.h"
#include "okformat.h"
#include "pub3func.h"
#include "pescape.h"
#include "precycle.h"
#include "pub3eval.h"
#include "pub3parse.h"

//-----------------------------------------------------------------------

//static size_t depth;
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

//----------------------------------------------------------------------

namespace pub3 {

  //====================================================================

  // By default, we are already evaluated -- this is true for static
  // values like bools, strings, and integers.
  ptr<const expr_t> expr_t::eval_to_val (eval_t e) const 
  { return mkref (this); }

  //--------------------------------------------------------------------
  
  ptr<expr_t>
  expr_t::alloc (scalar_obj_t so)
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

  //-------------------------------------------------------------------

  str
  expr_t::safe_to_str (ptr<const expr_t> x, bool q)
  {
    str ret;
    if (!x) x = expr_null_t::alloc ();
    ret = x->to_str (true);
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<mref_t> expr_t::eval_to_ref (eval_t e) const
  { 
    ptr<expr_t> x;
    ptr<const expr_t> v = eval_to_val (e);
    if (v) x = v->copy ();
    ptr<const_mref_t> ret = const_mref_t::alloc (x);
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_t> expr_t::copy () const 
  { return expr_cow_t::alloc (mkref (this)); }
  ptr<expr_t> expr_t::deep_copy () const 
  { return expr_cow_t::alloc (mkref (this)); }

  //--------------------------------------------------------------------

  bool 
  expr_t::eval_as_bool (eval_t e) const
  {
    bool ret = false;
    bool l = e.set_silent (true);
    ptr<const expr_t> x = eval_to_val (e);
    e.set_silent (l);
    if (x) ret = x->to_bool ();
    return ret;
  }
  
  //---------------------------------------------------------------------

  str
  expr_t::eval_as_str (eval_t e) const
  {
    ptr<const expr_t> x = eval_to_val (e);
    str ret;
    if (x) ret = x->to_str ();
    return ret;
  }

  //---------------------------------------------------------------------

  ptr<const expr_dict_t>
  expr_t::eval_as_dict (eval_t e) const
  {
    ptr<const expr_t> x = eval_to_val (e);
    ptr<const expr_dict_t> ret;
    if (x) { ret = x->to_dict (); }
    return ret;
  }


  //---------------------------------------------------------------------
  
  ptr<rxx>
  expr_t::str2rxx (const eval_t *e, const str &b, const str &o) const
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

  //---------------------------------------------------------------------

  scalar_obj_t expr_t::to_scalar () const { return to_str (); }
  
  //---------------------------------------------------------------------

  ptr<expr_t>
  expr_t::safe_expr (ptr<expr_t> x)
  {
    if (!x) x = expr_null_t::alloc ();
    return x;
  }
  
  //---------------------------------------------------------------------

  bool
  expr_t::eval_as_null (eval_t e) const
  {
    bool ret = true;
    bool l = e.set_silent (true);
    ptr<const expr_t> x = eval_to_val (e);
    e.set_silent (l);
    if (x) ret = x->is_null ();
    return ret;
  }

  //---------------------------------------------------------------------

  scalar_obj_t
  expr_t::eval_as_scalar (eval_t e) const
  {
    scalar_obj_t ret;
    ptr<const expr_t> x = eval_to_val (e);
    if (x) { ret = x->to_scalar (); }
    return ret;
  }

  //====================================================================

  ptr<expr_t>
  expr_cow_t::mutable_ptr ()
  {
    if (_orig) {
      assert (!_copy);
      _copy = _orig->deep_copy ();
      _orig = NULL;
    }
    return _copy;
  }

  //--------------------------------------------------------------------

  ptr<const expr_t> 
  expr_cow_t::const_ptr () const
  {
    assert (!_copy || !_orig);
    ptr<const expr_t> ret;
    if (_copy) ret = _copy;
    else if (_orig) ret = _orig;
    return ret;
  }
  
  //--------------------------------------------------------------------

  ptr<expr_dict_t> 
  expr_cow_t::to_dict ()
  {
    ptr<expr_dict_t> r;
    ptr<expr_t> x = mutable_ptr ();
    if (x) r = x->to_dict ();
    return r;
  }


  //--------------------------------------------------------------------

  ptr<const expr_dict_t>
  expr_cow_t::to_dict () const
  {
    ptr<const expr_dict_t> r;
    ptr<const expr_t> x = const_ptr ();
    if (x) r = x->to_dict ();
    return r;
  }

  //--------------------------------------------------------------------

  ptr<expr_list_t> 
  expr_cow_t::to_list () 
  {
    ptr<expr_list_t> r;
    ptr<expr_t> x = mutable_ptr ();
    if (x) r = x->to_list ();
    return r;
  }

  //--------------------------------------------------------------------

  ptr<const expr_list_t>
  expr_cow_t::to_list () const
  {
    ptr<const expr_list_t> r;
    ptr<const expr_t> x = const_ptr ();
    if (x) r = x->to_list ();
    return r;
  }

  //====================================================================

  ptr<expr_t> 
  mref_dict_t::get_value () 
  { 
    ptr<expr_t> ret;
    ptr<expr_t> *xp =(*_dict)[_slot]; 
    if (xp) { ret = *xp; }
    return ret;
  }

  //--------------------------------------------------------------------

  bool 
  mref_dict_t::set_value (ptr<expr_t> x) 
  { 
    _dict->insert (_slot, x); 
    return true;
  }

  //====================================================================
  
  ptr<expr_t> mref_list_t::get_value () { return _list->lookup (_index); }

  //-----------------------------------------------------------------------

  bool
  mref_list_t::set_value (ptr<expr_t> x) 
  { 
    _list->set (_index, x); 
    return true;
  }
  
  //-----------------------------------------------------------------------
  
  ptr<mref_dict_t> mref_dict_t::alloc (ptr<bindtab_t> b, const str &n)
  { return New refcounted<mref_dict_t> (b, n); }

  //====================================================================

  // For constants, we can't change them anyway, so copy's are no-ops
  ptr<expr_t> expr_constant_t::copy () const 
  { return mkref (const_cast<expr_constant_t *> (this)); }
  ptr<expr_t> expr_constant_t::deep_copy () const 
  { return mkref (const_cast<expr_constant_t *> (this)); }
  
  //====================================================================

  ptr<expr_null_t>
  expr_null_t::alloc ()
  {
    static ptr<expr_null_t> s_null;
    if (!s_null) {
      s_null = New refcounted<expr_null_t> ();
    }
    return s_null;
  }

  //====================================================================
  
  scalar_obj_t
  expr_bool_t::to_scalar () const
  {
    scalar_obj_t so;
    so.set_i (_b);
    return so;
  }
  
  //--------------------------------------------------------------------
  
  str expr_bool_t::static_to_str (bool b) { return b ? "true" : "false"; }
  str expr_bool_t::to_str (bool q) const { return to_str (_b); }
  
  //--------------------------------------------------------------------
  
  ptr<expr_bool_t> expr_bool_t::_false;
  ptr<expr_bool_t> expr_bool_t::_true;
  
  //---------------------------------------------------------------------

  ptr<expr_bool_t>
  expr_bool_t::alloc (bool b)
  {
    ptr<expr_bool_t> &retref = b ? _true : _false;
    if (!retref) { retref = New refcounted<expr_bool_t> (b); }
    return retref;
  }

  //====================================================================

  ptr<const expr_t>
  expr_logical_t::eval_to_val (eval_t e) const
  {
    return expr_bool_t::alloc (eval_logical (e));
  }

  //====================================================================

  ptr<expr_OR_t>
  expr_OR_t::alloc (ptr<expr_t> t1, ptr<expr_t> t2) 
  {
    return New refcounted<expr_OR_t> (t1, t2, plineno ());
  }

  //--------------------------------------------------------------------

  bool
  expr_OR_t::eval_logical (eval_t e) const
  {
    bool ret = false;
    ret = ((_t1 && _t1->eval_as_bool (e)) || (_t2 && _t2->eval_as_bool (e)));
    return ret;
  }

  //====================================================================
  
  bool
  expr_AND_t::eval_logical (eval_t e) const
  {
    bool ret;
    ret = ((_f1 && _f1->eval_as_bool (e)) && (_f2 && _f2->eval_as_bool (e)));
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_AND_t>
  expr_AND_t::alloc (ptr<expr_t> f1, ptr<expr_t> f2)
  {
    return New refcounted<expr_AND_t> (f1, f2, plineno ());
  }

  //====================================================================
  
  bool
  expr_NOT_t::eval_logical (eval_t e) const
  {
    bool ret = true;
    if (_e) ret = !(_e->eval_as_bool (e));
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_NOT_t>
  expr_NOT_t::alloc (ptr<expr_t> x)
  {
    return New refcounted<expr_NOT_t> (x, plineno ());
  }

  //====================================================================
  
  bool
  expr_EQ_t::eval_logical (eval_t e) const
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

  //--------------------------------------------------------------------

  ptr<expr_EQ_t>
  expr_EQ_t::alloc (ptr<expr_t> o1, ptr<expr_t> o2, bool pos)
  {
    return New refcounted<expr_EQ_t> (o1, o2, pos, plineno ());
  }

  //====================================================================
  
  bool
  expr_relation_t::eval_logical (eval_t e) const
  {
    bool ret = false;
    if (_l && !_l->eval_as_null (e) && _r && !_r->eval_as_null (e)) {
      scalar_obj_t l = _l->eval_as_scalar (e);
      scalar_obj_t r = _r->eval_as_scalar (e);

      // perform relations as scalars, to accommdate double v. int,
      // string v. double, etc...
      switch (_op) {
      case XPUB3_REL_LT : ret = (l < r);  break;
      case XPUB3_REL_GT : ret = (l > r);  break;
      case XPUB3_REL_LTE: ret = (l <= r); break;
      case XPUB3_REL_GTE: ret = (l >= r); break;
      default: panic ("unexpected relational operator!\n");
      }
    } else {
      report_error (e, "one or more relational arguments were null");
    }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_relation_t>
  expr_relation_t::alloc (ptr<expr_t> o1, ptr<expr_t> o2, xpub3_relop_t op)
  {
    return New refcounted<expr_relation_t> (o1, o2, op, plineno ());
  }

  //====================================================================

  ptr<const expr_t> 
  expr_add_t::eval_to_val (eval_t e) const
  {
    ptr<expr_t> out;
    ptr<const expr_t> e1, e2;
    ptr<const expr_list_t> l1, l2;
    ptr<const expr_dict_t> d1, d2;
    str s1, s2;

    const char *op = _pos ? "addition" : "subtraction";

    if (!_t1 || !(e1 = _t1->eval_to_val (e))) {
      report_error (e, strbuf ("left-hand term of %s evaluates to null", op));
    } else if (!_t2 || !(e2 = _t2->eval_to_val (e))) {
      report_error (e, strbuf ("right-hand term of %s evaluates to null", op));

      // Two lists added (but not subtracted)
    } else if ((l1 = e1->to_list ()) || (l2 = e2->to_list ())) {
      if (!l1 || !l2) {
	report_error (e, "addition on lists only works with 2 lists");
      } else if (!_pos) { 
	report_error (e, "cannot subtract lists");
      } else {
	ptr<expr_list_t> s = New refcounted<expr_list_t> (_lineno);
	*s += *l1;
	*s += *l2;
	out = s;
      }

      // Two dicts added or subtracted
    } else if ((d1 = e1->to_dict ()) || (d2 = e2->to_dict ())) {
      if (!d1 || !d2) {
	report_error (e, "addition on dicts only work with 2 dicts");
      } else {
	ptr<expr_dict_t> d = New refcounted<expr_dict_t> (*d1);
	if (_pos) {
	  *d += *d2;
	} else {
	  *d -= *d2;
	}
	out = d;
      }

    } else {
      scalar_obj_t s1 = e1->to_scalar ();
      scalar_obj_t s2 = e2->to_scalar ();
      
      scalar_obj_t res;
      if (_pos) { res = s1 + s2; }
      else      { res = s1 - s2; }

      out = expr_t::alloc (res);
    }

    return out;
  }

  //--------------------------------------------------------------------

  ptr<expr_add_t> expr_add_t::alloc (ptr<expr_t> x1, ptr<expr_t> x2, bool pos)
  { return New refcounted<expr_add_t> (x1, x2, pos, plineno ()); }

  //====================================================================

  ptr<const expr_t>
  expr_mult_t::eval_to_val (eval_t e) const
  {
    ptr<const expr_t> ret;
    ptr<const expr_t> e1, e2;
    ptr<const expr_list_t> l;
    int64_t n;
    
    if (!_f1 || !(e1 = _f1->eval_to_val (e)) || e1->is_null ()) {
      report_error (e, "mult: left-hand factor was NULL");
    } else if (!_f2 || !(e2 = _f2->eval_to_val (e)) || e2->is_null ()) {
      report_error (e, "mult: right-hand factor was NULL");
    } else if (e1->to_dict () || e2->to_dict ()) {
      report_error (e, "cannot multiply dictionaries");
    } else if ((l = e1->to_list ()) && e2->to_int (&n) && n < 0x100) {
      ptr<expr_list_t> lo = New refcounted<expr_list_t> (_lineno);
      for (int64_t i = 0; i < n; i++) {
	*lo += *l;
      }
      ret = lo;
    } else if (e1->to_list () || e2->to_list ()) {
      report_error (e, "can only multiply lists by small integers");
    } else {
      scalar_obj_t o1 = _f1->to_scalar ();
      scalar_obj_t o2 = _f2->to_scalar ();
      
      if (o1.is_null ()) {
	report_error (e, "mult: left-hand factor was not multipliable");
      } else if (o2.is_null ()) {
	report_error (e, "mutl: right-hand factor was not multipliable");
      } else {
	scalar_obj_t out = o1 * o2;
	if (out.is_null ()) {
	  report_error (e, "mult: no plausible evaluation found!");
	} else {
	  ret = expr_t::alloc (out);
	}
      }
    }
    return ret;
  }

  //====================================================================

  ptr<expr_mod_t>
  expr_mod_t::alloc (ptr<expr_t> d, ptr<expr_t> n)
  {
    return New refcounted<expr_mod_t> (d, n, plineno ());
  }

  //====================================================================

  ptr<expr_div_t>
  expr_div_t::alloc (ptr<expr_t> d, ptr<expr_t> n)
  {
    return New refcounted<expr_div_t> (d, n, plineno ());
  }

  //====================================================================

  ptr<expr_mult_t>
  expr_mult_t::alloc (ptr<expr_t> f1, ptr<expr_t> f2)
  {
    return New refcounted<expr_mult_t> (f1, f2, plineno ());
  }

  //====================================================================

  ptr<const expr_t>
  expr_div_or_mod_t::eval_to_val (eval_t e) const
  {
    ptr<expr_t> ret;
    scalar_obj_t o = eval_internal (e);
    if (!o.is_null ()) { ret = expr_t::alloc (o); }
    return ret;
  }

  //--------------------------------------------------------------------

  scalar_obj_t
  expr_div_or_mod_t::eval_as_scalar (eval_t e) const
  {
    return eval_internal (e);
  }

  //--------------------------------------------------------------------
  
  scalar_obj_t
  expr_div_or_mod_t::eval_internal (eval_t e) const
  {
    scalar_obj_t out;
    ptr<const expr_t> en, ed;
    scalar_obj_t n, d;
    const char *op = operation ();
    
    if (!_n || !(en = _n->eval_to_val (e))) {
      report_error (e, strbuf ("%s: numerator was NULL", op));
    } else if (!_d || !(ed = _d->eval_to_val (e))) {
      report_error (e, strbuf ("%s: denominator was NULL", op));
    } else if ((n = en->to_scalar ()).is_null ()) {
      report_error (e, strbuf ("%s: numerator was not a scalar", op));
    } else if ((d = ed->to_scalar ()).is_null ()) {
      report_error (e, strbuf ("%s: denominator was not a scalar", op));
    } else {
     
      if (div ()) { out = n / d; }
      else { out = n % d; }
      
      if (!out.is_null ()) {
	/* good! */
      } else if (out.is_inf ()) {
	report_error (e, strbuf ("%s by zero", op));
      } else {
	report_error (e, strbuf ("can't perform %s on strings", op));
      }
    }
    return out;
  }

  //====================================================================
  // see pub3ref.C for implementation of ref classes
  //====================================================================

  bool
  expr_varref_or_rfn_t::unshift_argument (ptr<expr_t> x)
  {
    if (!_arglist) {
      _arglist = New refcounted<expr_list_t> (_lineno);
    }
    _arglist->push_front (x);
    return true;
  }

  //-----------------------------------------------------------------------

  ptr<expr_varref_or_rfn_t> 
  expr_varref_or_rfn_t::alloc (const str &l)
  {
    return New refcounted<expr_varref_or_rfn_t> (l, plineno ());
  }
  
  //-----------------------------------------------------------------------

  ptr<const expr_t>	
  expr_varref_or_rfn_t::eval_to_val (eval_t e) const
  {
    ptr<const expr_t> r, v;
    ptr<const proc_call_t> fnd;
    ptr<const expr_t> rfn;
    bool make_silent = _arglist;

    bool old_silent = e.set_silent (make_silent);
    v = expr_varref_t::eval_to_val (e);
    e.set_silent (old_silent);

    if (!_arglist) { r = v; }
    else if (v && (fnd = v->to_proc_call ())) {
      r = fnd->eval_to_val (e, _arglist);
    } else if ((rfn = get_rfn ())) {
      r = rfn->eval_to_val (e);
    }
    return r;
  }
  
  //-----------------------------------------------------------------------

  ptr<mref_t>	
  expr_varref_or_rfn_t::eval_to_ref (eval_t e) const
  {
    ptr<mref_t> r, v;
    ptr<expr_t> x;
    ptr<const proc_call_t> fnd;
    ptr<const expr_t> rfn;
    bool make_silent = _arglist;

    bool old_silent = e.set_silent (make_silent);
    v = expr_varref_t::eval_to_ref (e);
    e.set_silent (old_silent);

    if (!_arglist) { r = v; }
    else if (v && (x = v->get_value ()) && (fnd = x->to_proc_call ())) { 
      r = fnd->eval_to_ref (e, _arglist); 
    } else if ((rfn = get_rfn ())) {
      r = rfn->eval_to_ref (e);
    }
    return r;
  }
  
  //-----------------------------------------------------------------------

  ptr<const expr_t>
  expr_varref_or_rfn_t::get_rfn () const
  {
    if (_arglist && !_rfn) {
      _rfn = rfn_factory_t::get ()->alloc (_name, _arglist, _lineno);
    }
    return _rfn;
  }
  
  //====================================================================

  bool expr_strbuf_t::to_bool () const { return _b.len () > 0; }
  scalar_obj_t expr_strbuf_t::to_scalar () const { return scalar_obj_t (_b); }
  bool expr_strbuf_t::to_null () const { return false; }
  bool expr_strbuf_t::to_len (size_t *s) const { *s = _b.len (); return true; }

  //--------------------------------------------------------------------

  str
  expr_strbuf_t::to_str (bool q) const 
  { 
    str ret = q ? json::quote (_b) : str (_b);
    return ret; 
  }

  //--------------------------------------------------------------------

  ptr<rxx>
  expr_strbuf_t::to_regex () const
  {
    ptr<rxx> ret;
    if (_b.len ()) ret = str2rxx (NULL, _b, NULL);
    return ret;
  }

  //--------------------------------------------------------------------
  
  ptr<expr_strbuf_t>
  expr_strbuf_t::alloc (const str &s)
  {
    return New refcounted<expr_strbuf_t> (s, plineno ());
  }

  //--------------------------------------------------------------------

  void expr_strbuf_t::add (char ch) { _b.fmt ("%c", ch); }

  //--------------------------------------------------------------------

  void expr_strbuf_t::add (str s) 
  {
    if (s) {
      _hold.push_back (s);
      _b << s;
    }
  }

  //====================================================================

  bool expr_str_t::to_bool () const { return (_val && _val.len ()); }
  scalar_obj_t expr_str_t::to_scalar () const { return scalar_obj_t (_val); }
  bool expr_str_t::to_null () const { return !_val; }

  //--------------------------------------------------------------------

  str
  expr_str_t::to_str (bool q) const 
  { 
    str ret = q ? json::quote (_val) : _val;
    return ret; 
  }

  //--------------------------------------------------------------------
  
  bool
  expr_str_t::to_len (size_t *s) const
  {
    *s = _val ? _val.len () : 0;
    return true;
  }
  
  //--------------------------------------------------------------------

  ptr<rxx>
  expr_str_t::to_regex () const
  {
    ptr<rxx> ret;
    if (_val) { ret = str2rxx (NULL, _val, NULL); }
    return ret;
  }

  //--------------------------------------------------------------------
  
  ptr<expr_str_t> expr_str_t::alloc (str s) 
  { return New refcounted<expr_str_t> (s); }
  
  //====================================================================
  
  static recycler_t<expr_int_t> _int_recycler (1000);
  
  //-----------------------------------------------------------------------
  
  ptr<expr_int_t>
  expr_int_t::alloc (int64_t i)
  {
    return _int_recycler.alloc (i);
  }
  
  //-----------------------------------------------------------------------
  
  void
  expr_int_t::finalize ()
  {
    _int_recycler.recycle (this);
  }
  
  //-----------------------------------------------------------------------

  scalar_obj_t
  expr_int_t::to_scalar () const
  {
    scalar_obj_t so;
    so.set (_val);
    return so;
  }

  //-----------------------------------------------------------------------
  
  u_int64_t
  expr_int_t::to_uint () const
  {
    u_int64_t out = 0;
    to_uint (&out);
    return out;
  }
  
  //-----------------------------------------------------------------------
  
  bool
  expr_int_t::to_uint (u_int64_t *u) const
  {
 
    bool r = false;
    if (_val >= 0) {
      *u = _val;
      r = true;
    }
    return r;
  }
  
  //====================================================================
  
  scalar_obj_t
  expr_uint_t::to_scalar () const
  {
    scalar_obj_t so;
    so.set_u (_val);
    return so;
  }

  //-----------------------------------------------------------------------
  
  int64_t
  expr_uint_t::to_int () const
  {
    int64_t out = 0;
    to_int (&out);
    return out;
  }
  
  //-----------------------------------------------------------------------
  
  bool
  expr_uint_t::to_int (int64_t *out) const
  {
    bool ret = false;
    if (_val <= u_int64_t (INT64_MAX)) {
      ret = true;
      *out = _val;
    }
    return ret;
  }

  //====================================================================
  
  scalar_obj_t
  expr_double_t::to_scalar () const
  {
    scalar_obj_t so;
    so.set (_val);
    return so;
  }

  //====================================================================

  bool
  expr_list_t::is_static () const
  {
    if (_static.is_set ()) { return _static.value (); }
    bool sttc = true;
    for (size_t i = 0; sttc && i < vec_base_t::size (); i++) {
      ptr<const expr_t> x;
      x = (*this)[i];
      if (x && !x->is_static ()) sttc = false;
    }
    _static.set (sttc);
    return sttc;
  }

  //--------------------------------------------------------------------

  void
  expr_list_t::push_front (ptr<expr_t> e)
  {
    // XXX - hack -- push_back and then bubble to the front!
    push_back (e);
    for (size_t i = size () - 1; i > 0; i--) {
      ptr<expr_t> tmp = (*this)[i];
      (*this)[i] = (*this)[i-1];
      (*this)[i-1] = tmp;
    }
  }

  //--------------------------------------------------------------------

  ptr<expr_list_t> expr_list_t::alloc ()
  { return New refcounted<expr_list_t> (plineno ()); } 

  //--------------------------------------------------------------------

  ptr<expr_t> 
  expr_list_t::deep_copy () const
  {
    ptr<expr_list_t> out;
    size_t l = size ();
    for (size_t i = 0; i < l; i++) {
      ptr<expr_t> nv = (*this)[i]->copy ();
      out->push_back (nv);
    }
    return out;
  }
  
  //--------------------------------------------------------------------

  bool
  expr_list_t::to_len (size_t *s) const
  {
    *s = size ();
    return true;
  }

  //---------------------------------------------------------------------

  scalar_obj_t 
  expr_list_t::to_scalar () const
  {
    scalar_obj_t so = scalar_obj_t (to_str ());
    return so;
  }
  
  //--------------------------------------------------------------------

  bool
  expr_list_t::fixup_index (ssize_t *ip, bool lax) const
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

  //--------------------------------------------------------------------
  
  ptr<const expr_t>
  expr_list_t::lookup (ssize_t s, bool *ibp) const
  {
    ptr<const expr_t> r;
    bool ib;
    if ((ib = fixup_index (&s))) {
      r = (*this)[s];
    }
    
    if (ibp) *ibp = ib;
    return r;
  }
  
  //---------------------------------------------------------------------

  ptr<expr_t>
  expr_list_t::lookup (ssize_t s, bool *ibp)
  {
    ptr<expr_t> r;
    bool ib;
    
    if ((ib = fixup_index (&s))) {
      r = (*this)[s];
    }
    
    if (ibp) *ibp = ib;
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<expr_t> &
  expr_list_t::push_back (ptr<expr_t> x)
  {
    return vec_base_t::push_back (expr_t::safe_expr (x));
  }

  //-----------------------------------------------------------------------
  
  void
  expr_list_t::set (ssize_t i, ptr<expr_t> v)
  {
    if (fixup_index (&i, true)) {
      while (i >= ssize_t (vec_base_t::size ())) {
	vec_base_t::push_back (NULL);
      }
      (*this)[i] = expr_t::safe_expr (v);
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
  expr_list_t::to_str (bool q) const
  {
    vec<str> v;
    size_t sz = size ();
    for (size_t i = 0; i < sz; i++) {
      v.push_back (expr_t::safe_to_str ((*this)[i]));
    }
    str ret = vec2str (v, '[', ']');
    return ret;
  }
  
  //--------------------------------------------------------------------
  
  ptr<rxx>
  expr_list_t::to_regex (const eval_t *e) const
  {
    str opts, body;
    ptr<rxx> ret;
    ptr<const expr_t> x;
    
    if (size () >= 2 && (x = (*this)[1])) { opts = x->to_str (); }
    if (size () >= 1 && (x = (*this)[0])) { body = x->to_str (); }
    if (body) { ret = str2rxx (e, body, opts); }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_list_t::eval_to_ref (eval_t e) const
  {
    bool sttc = true;
    size_t l = vec_base_t::size ();

    for (size_t i = 0; sttc && i < l ; i++) {
      if ((*this)[i] && (*this)[i]->is_static ()) {
	sttc = false;
      }
    }
    ptr<expr_t> out;

    if (sttc) {
      out = expr_cow_t::alloc (mkref (this));
    } else {
      ptr<expr_list_t> nl = New refcounted<expr_list_t> ();
      for (size_t i = 0; i < l; i++) {
	ptr<expr_t> nv;
	ptr<mref_t> r;
	ptr<expr_t> value = (*this)[i];

	if (!value || !(r = value->eval_to_ref (e)) || 
	    !(nv = r->get_value ())) {
	  nv = expr_null_t::alloc ();
	}
	nl->push_back (nv);
      }
      out = nl;
    }
    ptr<const_mref_t> ret = New refcounted<const_mref_t> (out);
    return ret;
  }
  
  //====================================================================

  static rxx_factory_t g_rxx_factory;

  //--------------------------------------------------------------------

  ptr<rxx>
  rxx_factory_t::compile (str body, str opts, str *errp)
  {
    return g_rxx_factory._compile (body, opts, errp);
  }

  //--------------------------------------------------------------------

  ptr<rxx>
  rxx_factory_t::_compile (str body, str opts, str *errp)
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
  
  //====================================================================

  expr_regex_t::expr_regex_t (int lineno) : expr_t (lineno) {}

  //--------------------------------------------------------------------

  ptr<expr_regex_t> expr_regex_t::alloc (ptr<rxx> x, str b, str o)
  { return New refcounted<expr_regex_t> (x, b, o, plineno ()); }

  //--------------------------------------------------------------------

  expr_regex_t::expr_regex_t (ptr<rxx> x, str b, str o, int l)
    : expr_t (l), _rxx (x), _body (b), _opts (o) {}

  //====================================================================

  ptr<expr_shell_str_t> expr_shell_str_t::alloc (str s)
  {
    if (!s) s = "";
    return New refcounted<expr_shell_str_t> (s, plineno ());
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_shell_str_t::eval_to_val (eval_t e) const
  {
    ptr<expr_t> out;

    if (_els) {
      vec<str> hold;
      strbuf b;
      size_t sz = _els->size ();
      bool ok = true;

      for (size_t i = 0; ok && i < sz; i++) {
	ptr<const expr_t> x = (*_els)[i];
	if (!x) { x = expr_null_t::alloc (); }
	x = x->eval_to_val (e);
	if (!x) { x = expr_null_t::alloc (); }
	str s = x->to_str (false);
	hold.push_back (s);
	b << s;
	
      }
      if (ok) { out = expr_str_t::alloc (b); }
    }
    
    if (!out) { out = expr_str_t::alloc (expr_null_t::alloc ()->to_str ()); }
    
    return out;
  }
  
  //-----------------------------------------------------------------------

  void
  expr_shell_str_t::make_str (strbuf *b, vec<str> *v)
  {
    if (b->tosuio ()->resid ()) {
      _els->push_back (New refcounted<expr_str_t> (*b));
      b->tosuio ()->clear ();
      v->setsize (0);
    }
  }
  
  //-----------------------------------------------------------------------

  ptr<expr_t>
  expr_shell_str_t::compact () const
  {
    str s;
    ptr<expr_t> ret;
    
    if (_els->size () == 1 && (*_els)[0]->to_str ()) {
      ret = (*_els)[0];
    } else {
      ptr<expr_shell_str_t> out = New refcounted<expr_shell_str_t> (_lineno);
    
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
  
  expr_shell_str_t::expr_shell_str_t (int lineno)
    : expr_t (lineno), 
      _els (expr_list_t::alloc (lineno))  {}
  
  //----------------------------------------------------------------------
  
  expr_shell_str_t::expr_shell_str_t (const str &s, int lineno)
    : expr_t (lineno), 
      _els (expr_list_t::alloc (lineno)) 
  { _els->push_back (New refcounted<expr_str_t> (s)); }
  
  //----------------------------------------------------------------------
  
  expr_shell_str_t::expr_shell_str_t (ptr<expr_t> e, int lineno)
    : expr_t (lineno),
      _els (expr_list_t::alloc (lineno)) 
  { _els->push_back (e); }
  
  //====================================================================

  ptr<bindtab_t>
  cow_bindtab_t::mutate ()
  {
    ptr<bindtab_t> out;
    bindtab_t::const_iterator_t it (*_tab);
    ptr<expr_t> x;
    const str *keyp;

    while ((keyp = it.next (&x))) {
      ptr<expr_t> np;
      if (x) np = x->copy ();
      out->insert (*keyp, np);
    }
    return out;
  }

  //--------------------------------------------------------------------

  ptr<cow_bindtab_t> cow_bindtab_t::alloc (ptr<const bindtab_t> x)
  { return New refcounted<cow_bindtab_t> (x); }

  //--------------------------------------------------------------------

  bool cow_bindtab_t::lookup (const str &nm, ptr<const expr_t> *x) const
  { return _tab->lookup (nm, x); }

  //====================================================================

  void
  bindtab_t::overwrite_with (const bindtab_t &t)
  {
    bindtab_t::const_iterator_t it (t);
    const str *k;
    ptr<expr_t> v;
    while ((k = it.next (&v))) { 
      ptr<expr_t> nv;
      if (v) nv = v->copy ();
      insert (*k, nv); 
    }
  }

  //-----------------------------------------------------------------------

  bindtab_t &
  bindtab_t::operator+= (const bindtab_t &t)
  {
    overwrite_with (t);
    return *this;
  }

  //--------------------------------------------------------------------

  bindtab_t &
  bindtab_t::operator-= (const bindtab_t &in) 
  {
    bindtab_t::const_iterator_t it (in);
    const str *key;
    while ((key = it.next ())) { remove (*key); }
    return *this;
  }

  //--------------------------------------------------------------------

  bool
  bindtab_t::lookup (const str &nm, ptr<const expr_t> *outp) const
  {
    const ptr<expr_t> *xp = (*this)[nm];
    bool ret = xp;
    if (outp && xp) { *outp = *xp; }
    else if (outp) { *outp  = NULL; }
    return ret;
  }

  //====================================================================

  void bindlist_t::add (binding_t b) { push_back (b); }

  //--------------------------------------------------------------------

  ptr<bindlist_t> bindlist_t::alloc () 
  { return New refcounted<bindlist_t> (plineno ()); }
  
  //====================================================================

  bool
  expr_dict_t::is_static () const
  {
    if (_static.is_set ()) { return _static.value (); }
    bool sttc = true;
    ptr<expr_t> value;
    const_iterator_t it (*this);
    while (it.next (&value) && sttc) {
      if (value && !value->is_static ()) sttc = false;
    }
    _static.set (sttc);
    return sttc;
  }

  //--------------------------------------------------------------------

  ptr<mref_t>
  expr_dict_t::eval_to_ref (eval_t e) const
  {
    const_iterator_t it (*this);
    const str *key;
    ptr<expr_t> value;
    ptr<expr_t> out;

    // First see if any keys are static.  Note that this computation
    // will be memoized, so it's fast enough to do a full DFS here.
    // For static objects, make a COW version
    if (is_static ()) {
      out = expr_cow_t::alloc (mkref (this));
    } else {

      // Otherwise, recurse --- evaluate next layer down...
      ptr<expr_dict_t> d = New refcounted<expr_dict_t> ();
      it.reset ();
      while ((key = it.next (&value))) {
	ptr<expr_t> nv;
	ptr<mref_t> r;
	if (value) { 
	  r = value->eval_to_ref (e);
	  if (r) nv = r->get_value ();
	}
	if (nv) {
	  d->insert (*key, nv);
	}
      }
      out = d;
    }

    ptr<const_mref_t> ret = New refcounted<const_mref_t> (out);
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_t>
  expr_dict_t::deep_copy () const
  {
    const_iterator_t it (*this);
    const str *key;
    ptr<expr_t> value;
    ptr<expr_dict_t> ret = New refcounted<expr_dict_t> ();

    while ((key = it.next (&value))) {
      ptr<expr_t> nv;
      if (value) nv = value->copy ();
      ret->insert (*key, nv);
    }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_dict_t> expr_dict_t::parse_alloc ()
  { return New refcounted<expr_dict_t> (plineno ()); }
  
  //--------------------------------------------------------------------

  ptr<expr_dict_t> expr_dict_t::alloc ()
  { return New refcounted<expr_dict_t> (); }

  //--------------------------------------------------------------------

  void
  expr_dict_t::add (binding_t p)
  {
    insert (p.name (), p.expr ());
  }

  //--------------------------------------------------------------------
  
  str
  expr_dict_t::to_str (bool q) const
  {
    vec<str> v;
    const_iterator_t it (*this);
    const str *key;
    ptr<expr_t> val;
    str ret;

    while ((key = it.next (&val))) {
      str vs = expr_t::safe_to_str (val);
      str ks = json::quote (*key);
      strbuf b ("%s : %s", ks.cstr (), vs.cstr ());
      v.push_back (b);
    }
    ret = vec2str (v, '{', '}');
    return ret;
  }

  //--------------------------------------------------------------------

  bool
  expr_dict_t::to_len (size_t *s) const 
  {
    *s = size ();
    return true;
  }
  
  //-----------------------------------------------------------------------
  
  void
  expr_dict_t::replace (const str &nm, ptr<expr_t> x)
  {
    insert (nm, x);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  expr_dict_t::lookup (str nm) const
  {
    const ptr<expr_t> *rp;
    ptr<const expr_t> r;
    if ((rp = (*this)[nm])) r = *rp;
    return r;
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  expr_dict_t::lookup (str nm) 
  {
    ptr<expr_t> r, *rp;
    if ((rp = (*this)[nm])) r = *rp;
    return r;
  }

  //====================================================================

  expr_assignment_t::expr_assignment_t (ptr<expr_t> lhs, ptr<expr_t> rhs,
					lineno_t lineno)
    : _lhs (lhs), _rhs (rhs), _lineno (lineno) {}

  //---------------------------------------------------------------------

  ptr<expr_assignment_t> 
  expr_assignment_t::alloc (ptr<expr_t> l, ptr<expr_t> r)
  { return New refcounted<expr_assignment_t> (l, r, plineno ()); }

  //---------------------------------------------------------------------
  
  ptr<mref_t>
  expr_assignment_t::eval_to_ref (eval_t e) const
  {
    ptr<mref_t> rhs = _rhs->eval_to_ref (e);
    ptr<mref_t> lhs = _lhs->eval_to_ref (e);
    ptr<mref_t> ret;
    ptr<expr_t> v;
    if (!lhs) {
      report_error (e, "error in assignment: LHS evaluates to null");
    } else if (!(v = rhs->get_value ())) {
      report_error (e, "error in assignment: RHS evaluates to null");
    } else if (!(lhs->set_value (v))) {
      report_error (e, "error in assignment: invalid LHS");
    } else {
      ret = rhs;
    }
    return ret;
  }
  
  //====================================================================

  ptr<expr_cow_t> expr_cow_t::alloc (ptr<const expr_t> x)
  { return New refcounted<expr_cow_t> (x); }

  //--------------------------------------------------------------------

  bool 
  expr_cow_t::to_xdr (xpub3_expr_t *x) const
  {
    ptr<const expr_t> p = const_ptr ();
    bool ret = false;
    if (p) { ret = p->to_xdr (x); } 
    return ret;
  }
  
  //=======================================================================

  void 
  expr_t::report_error (eval_t e, str n) const
  {
    e.report_error (n, _lineno);
  }

  //=======================================================================

};

//=======================================================================


