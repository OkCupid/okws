#include "okformat.h"
#include "parseopt.h"
#include "okformat.h"
#include "pub3expr.h"
#include "pub3func.h"
#include "pescape.h"
#include "precycle.h"
#include "pub3eval.h"
#include "pub3parse.h"
#include "pub3file.h"
#include "okdbg.h"
#include "okconst.h"


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
pub3::json::quote (const str &s, str_opt_t so)
{
  str ret; 
  if (!so.m_quoted) { ret = s; }
  else { ret = quote (s, so.m_utf8); }
  return ret;
}

//-----------------------------------------------------------------------

str
pub3::json::quote (const str &s, bool utf8)
{
  str ret;
  if (!s) {
    ret = _null;
  } else {
    ret = json_escape (s, true, utf8);
  }
  return ret;
}

//----------------------------------------------------------------------

namespace pub3 {

  //============================== expr_t ================================

  // By default, we are already evaluated -- this is true for static
  // values like bools, strings, and integers.
  ptr<const expr_t> expr_t::eval_to_val (eval_t *e) const 
  { return mkref (this); }

  //--------------------------------------------------------------------

  str expr_t::undef_str = "undef";

  //--------------------------------------------------------------------

  void expr_t::report_error (eval_t *e, str msg) const
  { e->report_error (msg, _lineno); }
  
  //--------------------------------------------------------------------

  ptr<expr_t>
  expr_t::alloc (scalar_obj_t so)
  {
    ptr<expr_t> ret;
    int64_t i;
    u_int64_t u;
    str s;
    double d;
    
    if (so.to_int64 (&i)) {
      ret = expr_int_t::alloc (i);
    } else if (so.to_uint64 (&u)) {
      ret = New refcounted<expr_uint_t> (u);
    } else if (so.to_double (&d)) {
      ret = New refcounted<expr_double_t> (d); 
    } else if ((s = so.to_str ())) {
      ret = New refcounted<expr_str_t> (s);
    } else {
      ret = New refcounted<expr_null_t> ();
    }
    return ret;
  }

  //-------------------------------------------------------------------

  str
  expr_t::safe_to_str (ptr<const expr_t> x, str_opt_t o)
  {
    str ret;
    if (!x) x = expr_null_t::alloc ();
    ret = x->to_str (o);
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<mref_t> expr_t::eval_to_ref (eval_t *e) const
  { 
    ptr<expr_t> x;
    ptr<const expr_t> v = eval_to_val (e);
    if (v) x = v->copy ();
    ptr<const_mref_t> ret = const_mref_t::alloc (x);
    return ret;
  }

  //--------------------------------------------------------------------
  
  ptr<expr_t>
  expr_t::eval_to_mval (eval_t *e) const
  {
    ptr<expr_t> ret;
    ptr<const expr_t> cx = eval_to_val (e);
    if (cx) { ret = cx->copy (); }
    return ret;
  }

  //--------------------------------------------------------------------
  
  ptr<expr_t> expr_t::copy () const 
  { return expr_cow_t::alloc (mkref (this)); }
  ptr<expr_t> expr_t::deep_copy () const 
  { return expr_cow_t::alloc (mkref (this)); }
  ptr<expr_t> expr_t::cow_copy () const { return NULL; }
  ptr<expr_t> expr_t::cast_hack_copy () const 
  { return mkref (const_cast<expr_t *> (this)); }

  //--------------------------------------------------------------------

  bool 
  expr_t::eval_as_bool (eval_t *e) const
  {
    bool ret = false;
    bool l = e->set_silent (true);
    ptr<const expr_t> x = eval_to_val (e);
    e->set_silent (l);
    if (x) ret = x->to_bool ();
    return ret;
  }
  
  //---------------------------------------------------------------------

  str
  expr_t::eval_as_str (eval_t *e) const
  {
    ptr<const expr_t> x = eval_to_val (e);
    str ret;
    if (x) {
      str_opt_t o (false, e->utf8_json ());
      ret = x->to_str (o);
    }
    return ret;
  }

  //---------------------------------------------------------------------

  ptr<bind_interface_t>
  expr_t::eval_to_bindtab (eval_t *e) const
  {
    ptr<const expr_dict_t> d = eval_as_dict (e);
    ptr<bind_interface_t> ret;
    if (d) { ret = cow_bindtab_t::alloc (d); }
    return ret;
  }

  //---------------------------------------------------------------------

  ptr<const expr_dict_t>
  expr_t::eval_as_dict (eval_t *e) const
  {
    ptr<const expr_t> x = eval_to_val (e);
    ptr<const expr_dict_t> ret;
    if (x) { ret = x->to_dict (); }
    return ret;
  }

  //---------------------------------------------------------------------
  
  ptr<rxx>
  expr_t::str2rxx (eval_t *e, const str &b, const str &o, str *errp) const
  {
    ptr<rxx> ret;
    if (b) {
      str err;
      ret = rxx_factory_t::compile (b, o, &err);
      if (e && !e->silent () && err) {
	report_error (e, err);
      }
      if (err && errp) *errp = err;
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

  ptr<expr_t>
  expr_t::safe_copy (ptr<const expr_t> x)
  {
    ptr<expr_t> ret;
    if (x) ret = x->copy ();
    if (!ret) ret = expr_null_t::alloc ();
    return ret;
  }

  //---------------------------------------------------------------------

  ptr<const expr_t>
  expr_t::safe_expr (ptr<const expr_t> x)
  {
    if (!x) x = expr_null_t::alloc ();
    return x;
  }
  
  //---------------------------------------------------------------------

  bool
  expr_t::eval_as_null (eval_t *e) const
  {
    bool ret = true;
    bool l = e->set_silent (true);
    ptr<const expr_t> x = eval_to_val (e);
    e->set_silent (l);
    if (x) ret = x->is_null ();
    return ret;
  }

  //---------------------------------------------------------------------

  scalar_obj_t
  expr_t::eval_as_scalar (eval_t *e) const
  {
    scalar_obj_t ret;
    ptr<const expr_t> x = eval_to_val (e);
    if (x) { ret = x->to_scalar (); }
    return ret;
  }

  //---------------------------------------------------------------------

  bool expr_t::might_block (ptr<const expr_t> x1, ptr<const expr_t> x2) 
  { 
    return (x1 && x1->might_block ()) || (x2 && x2->might_block ()); 
  }

  //---------------------------------------------------------------------

  bool 
  expr_t::might_block () const
  {
    if (!_might_block.is_set ()) {
      _might_block.set (might_block_uncached ()); 
    }
    return _might_block.value ();
  }

  //---------------------------------------------------------------------

  ptr<call_t>
  expr_t::coerce_to_call () 
  {
    ptr<call_t> ret;
    if (is_call_coercable ()) {
      ret = call_t::alloc (mkref (this), _lineno);
    }
    return ret;
  }

  //================================== expr_cow_t =========================

  ptr<const expr_t>
  expr_cow_t::eval_to_val (eval_t *e) const
  {
    ptr<const expr_t> r;
    ptr<const expr_t> x = const_ptr ();
    if (x) r = x->eval_to_val (e);
    return r;
  }

  //--------------------------------------------------------------------

  bool expr_cow_t::to_bool () const 
  { 
    ptr<const expr_t> x = const_ptr ();
    return x && x->to_bool ();
  }

  //--------------------------------------------------------------------

  bool expr_cow_t::is_null () const
  {
    ptr<const expr_t> x = const_ptr ();
    return !x || x->is_null ();
  }

  //--------------------------------------------------------------------

  bool expr_cow_t::might_block_uncached () const
  { return expr_t::might_block (const_ptr ()); }

  //-----------------------------------------------------------------------

  ptr<expr_t> expr_cow_t::copy () const { 
    ptr<expr_cow_t> new_cow = New refcounted<expr_cow_t> (*this); 

    // DK: The new copy should consider itself to be in the 'orig' state so
    // that expr_cow_t objects which are copied more than once do not
    // incorrectly point to the same object.
    //
    // Example: COW object A is copied to object B, then modified, setting the
    // _copy field in B.  COW object B is then copied to object C.  If object C
    // is now modified, object B would also see the changes without this fix.
    // With the fix, object C instead correctly copies itself again before
    // being modified.
    if (new_cow->_orig == NULL) {
      assert (_copy);
      new_cow->_orig = new_cow->_copy;
      new_cow->_copy = NULL;
    }
    return new_cow;
  }
  ptr<expr_t> expr_cow_t::deep_copy () const { return copy(); }
  ptr<expr_t> expr_cow_t::cow_copy () const { return copy(); }

  //-----------------------------------------------------------------------
  
  bool
  expr_cow_t::is_static () const
  {
    ptr<const expr_t> x = const_ptr ();
    return (x && x->is_static ());
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  expr_cow_t::mutable_ptr () 
  {
    if (_orig) {
      assert (!_copy);
      _copy = _orig->cow_copy ();
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

  //--------------------------------------------------------------------

  void
  expr_cow_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "expr:", const_ptr ());
  }

  //--------------------------------------------------------------------

  ptr<const callable_t>
  expr_cow_t::to_callable () const 
  {
    ptr<const callable_t> r;
    ptr<const expr_t> x = const_ptr ();
    if (x) r = x->to_callable ();
    return r;
  }

  //--------------------------------------------------------------------

  void
  expr_cow_t::propogate_metadata (ptr<const metadata_t> md)
  {
    ptr<expr_t> mp = mutable_ptr ();
    if (mp) { mp->propogate_metadata (md); }
  }

  //========================================= mref_dict_t ================

  ptr<expr_t> 
  mref_dict_t::get_value () 
  { 
    ptr<expr_t> ret;
    ptr<expr_t> *xp = (*_dict)[_slot]; 
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
  
  //-----------------------------------------------------------------------
  
  ptr<mref_dict_t> mref_dict_t::alloc (ptr<bindtab_t> b, const str &n)
  { return New refcounted<mref_dict_t> (b, n); }

  //======================================== mref_list_t =================
  
  ptr<expr_t> mref_list_t::get_value () { return _list->lookup (_index); }

  //-----------------------------------------------------------------------

  bool
  mref_list_t::set_value (ptr<expr_t> x) 
  { 
    _list->set (_index, x); 
    return true;
  }

  //-----------------------------------------------------------------------

  ptr<mref_list_t> mref_list_t::alloc (ptr<expr_list_t> d, ssize_t i)
  { return New refcounted<mref_list_t> (d, i); }

  //====================================================================

  // For constants, we can't change them anyway, so copy's are no-ops
  ptr<expr_t> expr_constant_t::copy () const { return cast_hack_copy (); }
  ptr<expr_t> expr_constant_t::deep_copy () const { return cast_hack_copy (); }
  ptr<expr_t> expr_constant_t::cow_copy () const { return cast_hack_copy (); }

  //--------------------------------------------------------------------

  void
  expr_constant_t::v_dump (dumper_t *d) const
  {
    str_opt_t o (true, false);
    str s = to_str (o);
    d->dump (s, true);
  }
  
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

  //-------------------------------------------------------------------

  void
  expr_null_t::v_dump (dumper_t *d) const
  {
    d->dump ("(null pub3 obj)", true);
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
  str expr_bool_t::to_str (str_opt_t o) const { return static_to_str (_b); }
  
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

  //---------------------------------------------------------------------
  
  ptr<expr_t> expr_bool_t::copy () const
  { return _b ? _true : _false; }

  //---------------------------------------------------------------------

  int64_t expr_bool_t::to_int () const { return _b; }
  u_int64_t expr_bool_t::to_uint () const { return _b; }
  bool expr_bool_t::to_uint (u_int64_t *u) const { *u = _b; return true; }
  bool expr_bool_t::to_int (int64_t *u) const { *u = _b; return true; }

  //====================================================================

  ptr<const expr_t>
  expr_logical_t::eval_to_val (eval_t *e) const
  {
    return expr_bool_t::alloc (eval_logical (e));
  }

  //-----------------------------------------------------------------------

  void 
  expr_logical_t::l_dump (dumper_t *d, ptr<const expr_t> a1, 
			  ptr<const expr_t> a2) const
  {
    s_dump (d, "left-arg", a1);
    s_dump (d, "right-arg", a2);
  }

  //====================================================================

  ptr<expr_OR_t>
  expr_OR_t::alloc (ptr<expr_t> t1, ptr<expr_t> t2) 
  {
    return New refcounted<expr_OR_t> (t1, t2, plineno ());
  }

  //--------------------------------------------------------------------

  bool
  expr_OR_t::eval_logical (eval_t *e) const
  {
    bool ret = false;
    ret = ((_t1 && _t1->eval_as_bool (e)) || (_t2 && _t2->eval_as_bool (e)));
    return ret;
  }

  //====================================================================
  
  bool
  expr_AND_t::eval_logical (eval_t *e) const
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
  expr_NOT_t::eval_logical (eval_t *e) const
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
  expr_EQ_t::eval_logical (eval_t *e) const
  {
    ptr<const expr_t> x1, x2;
    if (_o1) x1 = _o1->eval_to_val (e); 
    if (_o2) x2 = _o2->eval_to_val (e);
    return eval_final (x1, x2);
  }

  //--------------------------------------------------------------------

  bool
  expr_EQ_t::eval_final (ptr<const expr_t> x1, ptr<const expr_t> x2) const
  {
    return eval_static (x1, x2, _pos);
  }

  //--------------------------------------------------------------------

  static bool
  list_mania (ptr<const expr_t> x1, ptr<const expr_t> x2,
	      ptr<const expr_list_t> *l1p, ptr<const expr_list_t> *l2p) 
  {
    ptr<const expr_list_t> l1, l2;
    if (x1) { l1 = x1->to_list (); }
    if (x2) { l2 = x2->to_list (); }

    if (!l1 && !l2) { /* noop */ }
    else if (!l1) {
      assert (l2);
      ptr<expr_list_t> tmp = expr_list_t::alloc ();
      if (x1) { tmp->push_back (x1->copy ()); }
      l1 = tmp;
    } else if (!l2) {
      ptr<expr_list_t> tmp = expr_list_t::alloc ();
      if (x2) { tmp->push_back (x2->copy ()); }
      l2 = tmp;
    }

    *l1p = l1; *l2p = l2;
    bool ret = (l1 && l2);
    return ret;
  }

  //--------------------------------------------------------------------

  bool
  expr_EQ_t::eval_static (ptr<const expr_t> x1, ptr<const expr_t> x2, bool pos) 
  {
    int tmp = 0;
    int flip = pos ? 0 : 1;
    
    bool n1 = !x1 || x1->is_null ();
    bool n2 = !x2 || x2->is_null ();
    ptr<const expr_list_t> l1, l2;

    if (n1 && n2) { tmp = 1; }
    else if (n1 || n2) { tmp = 0; }
    else if (list_mania (x1, x2, &l1, &l2)) {
      tmp = (l1->size () == l2->size ());
      for (size_t i = 0; tmp && i < l1->size (); i++) {
	if (!eval_static ((*l1)[i], (*l2)[i], true)) { tmp = 0; }
      }
    } else {
      scalar_obj_t o1 = x1->to_scalar ();
      scalar_obj_t o2 = x2->to_scalar ();
      tmp = (o1 == o2) ? 1 : 0;
    } 
    bool ret = (tmp ^ flip);
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
  expr_relation_t::eval_logical (eval_t *e) const
  {
    ptr<const expr_t> l, r;
    if (_l) l = _l->eval_to_val (e);
    if (_r) r = _r->eval_to_val (e);
    return eval_final (e, l, r, _op, this);
  }

  //-----------------------------------------------------------------------

  bool
  expr_relation_t::eval_final (eval_t *e, ptr<const expr_t> l, 
			       ptr<const expr_t> r, xpub3_relop_t op,
			       const expr_t *self)
  {
    bool ret = false;
    ptr<const expr_list_t> ll, rl;

    if (list_mania (l, r, &ll, &rl)) {
     
      size_t ls, rs;
      ret = false;
      ls = ll->size ();
      rs = rl->size ();

      // Use python rules for lists of unequal size
      if (ls != rs) {
	switch (op) {
	case XPUB3_REL_LTE: case XPUB3_REL_LT: ret = (ls < rs); break;
	case XPUB3_REL_GTE: case XPUB3_REL_GT: ret = (ls > rs); break;
	default: break;
	}
      } else {
	
	// In the case of equality , we need to recurse...
	bool go = true;
	for (size_t i = 0; go && i < ls; i++) {
	  l = (*ll)[i]; 
	  r = (*rl)[i];
	  bool eq = expr_EQ_t::eval_static (l, r, true);
	  if (!eq) {
	    go = false;
	    ret = eval_final (e, l, r, op, self);
	  }
	}
	// If we made it to the end with equality, then we can return
	// return in the case of <= or >=, and false otherwise.
	if (go) {
	  ret = (op == XPUB3_REL_LTE || op == XPUB3_REL_GTE);
	}
      }

    } else if (l && r && !l->is_null () && !r->is_null ()) {

      scalar_obj_t sl = l->to_scalar ();
      scalar_obj_t sr = r->to_scalar ();

      // perform relations as scalars, to accommdate double v. int,
      // string v. double, etc...
      switch (op) {
      case XPUB3_REL_LT : ret = (sl < sr);  break;
      case XPUB3_REL_GT : ret = (sl > sr);  break;
      case XPUB3_REL_LTE: ret = (sl <= sr); break;
      case XPUB3_REL_GTE: ret = (sl >= sr); break;
      default: panic ("unexpected relational operator!\n");
      }
    } else if (e->opts () & P_WARN_RELARG_NULL) {
      self->report_error (e, "one or more relational arguments were null");
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

  bool expr_binaryop_t::might_block_uncached () const 
  { return expr_t::might_block (_o1, _o2); }

  //---------------------------------------------------------------------

  void
  expr_binaryop_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "left-arg", _o1);
    s_dump (d, "right-arg", _o2);
  }

  //---------------------------------------------------------------------

  ptr<const expr_t> 
  expr_binaryop_t::eval_to_val (eval_t *e) const
  {
    ptr<const expr_t> e1, e2;
    if (_o1) { e1 = _o1->eval_to_val (e); }
    if (_o2) { e2 = _o2->eval_to_val (e); }

    return eval_final (e, e1, e2);
  }

  //====================================================================

  ptr<const expr_t>
  expr_add_t::eval_final (eval_t *e, ptr<const expr_t> e1, 
			  ptr<const expr_t> e2) const
  {
    ptr<expr_t> out;
    ptr<const expr_list_t> l1, l2;
    ptr<const expr_dict_t> d1, d2;
    str s1, s2;

    const char *op = _pos ? "addition" : "subtraction";

    if (!e1 || e1->is_null ()) {
      report_error (e, strbuf ("left-hand term of %s evaluates to null", op));
    } else if (!e2 || e2->is_null ()) {
      report_error (e, strbuf ("right-hand term of %s evaluates to null", op));

      // Two lists added (but not subtracted)
    } else if ((l1 = e1->to_list ()) && (l2 = e2->to_list ())) {
      if (!_pos) { 
	report_error (e, "cannot subtract lists");
      } else {
	ptr<expr_list_t> s = New refcounted<expr_list_t> (_lineno);
	*s += *l1;
	*s += *l2;
	out = s;
      }

    } else if (e1->to_list () || e2->to_list ()) {
      report_error (e, "addition on lists only works with 2 lists");

      // Two dicts added or subtracted
    } else if ((d1 = e1->to_dict ()) && (d2 = e2->to_dict ())) {
      ptr<expr_dict_t> d = d1->copy_dict();
      if (_pos) {
	*d += *d2;
      } else {
	*d -= *d2;
      }
      out = d;

    } else if (e1->to_dict () || e2->to_dict ()) {
      report_error (e, "addition on dicts only work with 2 dicts");

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
  expr_mult_t::eval_final (eval_t *e, ptr<const expr_t> e1, 
			   ptr<const expr_t> e2) const
  {
    ptr<const expr_t> ret;
    ptr<const expr_list_t> l;
    int64_t n;

    if (!e1 || e1->is_null ()) {
      report_error (e, "mult: left-hand factor was NULL");
    } else if (!e2 || e2->is_null ()) {
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
      scalar_obj_t o1 = e1->to_scalar ();
      scalar_obj_t o2 = e2->to_scalar ();
      
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
  expr_div_or_mod_t::eval_final (eval_t *e, ptr<const expr_t> en, 
				    ptr<const expr_t> ed) const
  {
    ptr<const expr_t> ret;
    scalar_obj_t n, d;
    const char *op = operation ();

    if (!en || en->is_null ()) {
      report_error (e, strbuf ("%s: numerator was NULL", op));
    } else if (!ed || ed->is_null ()) {
      report_error (e, strbuf ("%s: denominator was NULL", op));
    } else if ((n = en->to_scalar ()).is_null ()) {
      report_error (e, strbuf ("%s: numerator was not a scalar", op));
    } else if ((d = ed->to_scalar ()).is_null ()) {
      report_error (e, strbuf ("%s: denominator was not a scalar", op));
    } else {
      scalar_obj_t out;
     
      if (div ()) { out = n / d; }
      else { out = n % d; }
      
      if (!out.is_null ()) {
	ret = expr_t::alloc (out);
      } else if (out.is_inf ()) {
	report_error (e, strbuf ("%s by zero", op));
      } else {
	report_error (e, strbuf ("can't perform %s on strings", op));
      }
    }
    return ret;
  }

  //====================================================================
  // see pub3ref.C for implementation of ref classes
  //====================================================================

  bool expr_strbuf_t::to_bool () const { return _b.len () > 0; }
  scalar_obj_t expr_strbuf_t::to_scalar () const { return scalar_obj_t (_b); }
  bool expr_strbuf_t::to_null () const { return false; }
  bool expr_strbuf_t::to_len (size_t *s) const { *s = _b.len (); return true; }

  //--------------------------------------------------------------------

  str
  expr_strbuf_t::to_str (str_opt_t o) const 
  { 
    str ret = json::quote (_b, o);
    return ret; 
  }

  //--------------------------------------------------------------------

  ptr<rxx>
  expr_strbuf_t::to_regex (str *errp) const
  {
    ptr<rxx> ret;
    if (_b.len ()) ret = str2rxx (NULL, _b, NULL, errp);
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
  expr_str_t::to_str (str_opt_t o) const 
  { 
    str ret = json::quote (_val, o);
    return ret; 
  }

  //--------------------------------------------------------------------
  
  bool
  expr_str_t::to_len (size_t *s) const
  {
    *s = _val ? utf8_len (_val) : 0;
    return true;
  }
  
  //--------------------------------------------------------------------

  ptr<rxx>
  expr_str_t::to_regex (str *errp) const
  {
    ptr<rxx> ret;
    if (_val) { ret = str2rxx (NULL, _val, NULL, errp); }
    return ret;
  }

  //-----------------------------------------------------------------------
  
  int64_t
  expr_str_t::to_int () const
  {
    scalar_obj_t so = to_scalar ();
    return so.to_int64 ();
  }
  
  //-----------------------------------------------------------------------
  
  bool
  expr_str_t::to_int (int64_t *i) const
  {
    scalar_obj_t so = to_scalar ();
    return so.to_int64 (i);
  }
  
  //-----------------------------------------------------------------------

  bool 
  expr_str_t::to_uint (u_int64_t *u) const
  {
    scalar_obj_t so = to_scalar ();
    return so.to_uint64 (u);
  }
  
  //-----------------------------------------------------------------------
  
  u_int64_t 
  expr_str_t::to_uint () const
  {
    scalar_obj_t so = to_scalar ();
    return so.to_uint64 ();
  }
  
  //--------------------------------------------------------------------
  
  ptr<expr_str_t> expr_str_t::alloc (str s) 
  { return New refcounted<expr_str_t> (s); }

  //--------------------------------------------------------------------

  ptr<expr_t> 
  expr_str_t::safe_alloc (str s)
  {
    ptr<expr_t> ret;
    if (s) ret = alloc (s);
    else ret = expr_null_t::alloc ();
    return ret;
  }
  
  //====================================================================
  
  static recycler_t<expr_int_t> _int_recycler(ok_pub3_recycle_limit_int, 
                                              "expr_int_t");

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
  
  //-----------------------------------------------------------------------

  str
  expr_int_t::to_str (str_opt_t o) const
  {
    str ret;
    bool js_safe = o.m_quoted;
    if (js_safe && ok_pub3_json_int_bitmax > 0) {
      int64_t x = 1;
      x = x << ok_pub3_json_int_bitmax;
      int64_t n = x * -1;
      if (_val >= x || _val <= n) {
	ret = strbuf ("\"%" PRId64 "\"", _val);
      }
    }
    if (!ret) {
      ret = strbuf ("%" PRId64, _val);
    }
    return ret;
  }

  double expr_int_t::to_double () const { return double (_val); }

  bool
  expr_int_t::to_double (double *out) const
  {
      *out = double(_val);
      return true;
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
 
  //-----------------------------------------------------------------------

  str
  expr_uint_t::to_str (str_opt_t o) const
  {
    str ret;
    bool js_safe = o.m_quoted;
    if (js_safe && ok_pub3_json_int_bitmax > 0) {
      u_int64_t x = 1;
      x = x << ok_pub3_json_int_bitmax;
      if (_val >= x) {
	ret = strbuf ("\"%" PRIu64 "\"", _val);
      }
    }
    if (!ret) {
      ret = strbuf ("%" PRIu64, _val);
    }
    return ret;
  }

  double expr_uint_t::to_double () const { return double (_val); }

  bool
  expr_uint_t::to_double (double *out) const
  {
      *out = double(_val);
      return true;
  }
 
  //====================================================================
  
  scalar_obj_t
  expr_double_t::to_scalar () const
  {
    scalar_obj_t so;
    so.set (_val);
    return so;
  }

  //-----------------------------------------------------------------------

  int64_t expr_double_t::to_int () const { return int64_t (_val); }

  //-----------------------------------------------------------------------

  bool expr_double_t::to_int (int64_t *out) const 
  { *out = int64_t (_val); return true; }

  //-----------------------------------------------------------------------
  
  str
  pub3::expr_double_t::to_str (str_opt_t o) const
  {
#define BUFSZ 128
    char buf[BUFSZ];
    snprintf (buf, BUFSZ, ok_double_fmt_ext_default, _val);
#undef BUFSZ
    return buf;
  }

  //-----------------------------------------------------------------------

  bool
  expr_double_t::to_double (double *out) const
  {
    *out = _val;
    return true;
  }
  
  //====================================================================

  void
  expr_list_t::v_dump (dumper_t *d) const
  {
    for (size_t i = 0; i < vec_base_t::size (); i++) {
      s_dump (d, "item:", (*this)[i]);
    }
  }

  //-----------------------------------------------------------------------

  void
  expr_list_t::propogate_metadata (ptr<const metadata_t> md)
  {
    for (size_t i = 0; i < vec_base_t::size (); i++) {
      ptr<expr_t> x = (*this)[i];
      if (x) { x->propogate_metadata (md); }
    }
  }

  //-----------------------------------------------------------------------
  
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

  bool
  expr_list_t::might_block_uncached () const
  {
    bool mb = false;
    for (size_t i = 0; !mb && i < vec_base_t::size (); i++) {
      ptr<const expr_t> x;
      x = (*this)[i];
      if (x && x->might_block()) mb = true;
    }
    return mb;
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

  ptr<expr_list_t> expr_list_t::parse_alloc ()
  { return New refcounted<expr_list_t> (plineno ()); } 

  //--------------------------------------------------------------------

  ptr<expr_list_t> expr_list_t::alloc ()
  { return New refcounted<expr_list_t> (); }

  //--------------------------------------------------------------------

  ptr<expr_t> 
  expr_list_t::deep_copy () const
  {
    ptr<expr_list_t> out = expr_list_t::alloc ();
    size_t l = size ();
    for (size_t i = 0; i < l; i++) {
      ptr<expr_t> nv = (*this)[i]->copy ();
      out->push_back (nv);
    }
    return out;
  }

  //--------------------------------------------------------------------

  ptr<expr_t> expr_list_t::cow_copy () const { return deep_copy (); }
  
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
  vec2str (const vec<str> &v, char o, char c, bool compact)
  {
    strbuf b ("%c", o);
    const char *sep = compact ? "," : ", ";
    for (size_t i = 0; i < v.size (); i++) {
      if (i != 0) b << sep;
      b << v[i];
    }
    b.fmt ("%c", c);
    return b;
  }
  
  //-----------------------------------------------------------------------
  
  str
  expr_list_t::to_str (str_opt_t o) const
  {
    vec<str> v;
    size_t sz = size ();
    o.m_quoted = true;
    for (size_t i = 0; i < sz; i++) {
      v.push_back (expr_t::safe_to_str ((*this)[i], o));
    }
    str ret = vec2str (v, '[', ']', o.compact ());
    return ret;
  }
  
  //--------------------------------------------------------------------
  
  ptr<rxx>
  expr_list_t::to_regex (eval_t *e) const
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

  ptr<const expr_t>
  expr_list_t::eval_to_val (eval_t *e) const
  { return eval_to_mval (e); }

  //--------------------------------------------------------------------

  ptr<expr_t>
  expr_list_t::eval_to_mval (eval_t *e) const
  {
    ptr<expr_t> out;
    if (is_static ()) {
      out = expr_cow_t::alloc (mkref (this));
    } else {
      size_t l = vec_base_t::size ();
      ptr<expr_list_t> nl = New refcounted<expr_list_t> ();
      for (size_t i = 0; i < l; i++) {
	ptr<expr_t> value = (*this)[i];
	ptr<expr_t> nv;
	if (!value || !(nv = value->eval_to_mval (e))) {
	  nv = expr_null_t::alloc ();
	}
	nl->push_back (nv);
      }
      out = nl;
    }
    return out;
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
    const char *b = body.cstr();
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
	b << "Cannot compile regex '" << body << "'";
	if (opts) {
	  b << " with options '" << opts << "'";
	}
	b << ": " << err << "\n"; 
	if (errp) *errp = b;
	tmp = NULL;
      } else {
	_cache.insert (k, tmp);
      }
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

  ptr<expr_shell_str_t> expr_shell_str_t::alloc ()
  { return New refcounted<expr_shell_str_t> (plineno ()); }

  //--------------------------------------------------------------------

  str
  expr_shell_str_t::to_str (str_opt_t o) const
  {
    strbuf b;
    for (size_t i = 0; _els && i < _els->size (); i++) {
      ptr<const expr_t> x = (*_els)[i];
      if (x) { 
	o.m_quoted = false; // never quote!
	str s = x->to_str (o);
	b << s;
      }
    }
    return b;
  }

  //--------------------------------------------------------------------

  void
  expr_shell_str_t::propogate_metadata (ptr<const metadata_t> md)
  {
    for (size_t i = 0; _els && i < _els->size (); i++) {
      ptr<expr_t> x = (*_els)[i];
      if (x) x->propogate_metadata (md);
    }
  }

  //--------------------------------------------------------------------

  bool
  expr_shell_str_t::might_block_uncached () const
  {
    bool ret = false;
    for (size_t i = 0; !ret && _els && i < _els->size (); i++) {
      ptr<const expr_t> x = (*_els)[i];
      if (x && x->might_block ()) ret = true;
    }
    return ret;
  }

  //--------------------------------------------------------------------

  void
  expr_shell_str_t::v_dump (dumper_t *d) const
  {
    if (_els) {
      for (size_t i = 0; i < _els->size (); i++) {
	s_dump (d, "item:", (*_els)[i]);
      }
    }
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_shell_str_t::eval_to_val (eval_t *e) const
  {
    ptr<expr_t> out;

    if (_els) {
      vec<str> hold;
      strbuf b;
      size_t sz = _els->size ();
      bool ok = true;

      for (size_t i = 0; ok && i < sz; i++) {
	ptr<const expr_t> x = (*_els)[i];
	str s;
	if (x) { x = x->eval_to_val (e); }
	if (x) { s = x->to_str (); }
	if (s) {
	  hold.push_back (s);
	  b << s;
	}
	
      }
      if (ok) { out = expr_str_t::alloc (b); }
    }
    
    if (!out) { out = expr_null_t::alloc (); }
    
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
  
  expr_shell_str_t::expr_shell_str_t (str s, int lineno)
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
    if (!_copy) {
      _copy = bindtab_t::alloc ();
      bindtab_t::const_iterator_t it (*_orig);
      ptr<expr_t> x;
      const str *keyp;
      
      while ((keyp = it.next (&x))) {
	ptr<expr_t> np;
	if (x) np = x->copy ();
	_copy->insert (*keyp, np);
      }
    }
    return _copy;
  }

  //--------------------------------------------------------------------
  
  ptr<const bindtab_t>
  cow_bindtab_t::tab () const
  {
    ptr<const bindtab_t> ret = _copy;
    if (!ret) ret = _orig;
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<cow_bindtab_t> cow_bindtab_t::alloc (ptr<const bindtab_t> x)
  { return New refcounted<cow_bindtab_t> (x); }

  //--------------------------------------------------------------------

  bool cow_bindtab_t::lookup (const str &nm, ptr<const expr_t> *x) const
  { return tab ()->lookup (nm, x); }

  //--------------------------------------------------------------------
  
  ptr<bindtab_t::const_iterator_t> cow_bindtab_t::iter () const 
  { return New refcounted<bindtab_t::const_iterator_t> (*tab ()); }

  //==================================== bind_interface_t ==============

  ptr<expr_dict_t>
  bind_interface_t::copy_to_dict () const
  {
    ptr<expr_dict_t> d = expr_dict_t::alloc ();
    ptr<bindtab_t::const_iterator_t> it = iter ();
    const str *key;
    ptr<expr_t> x;
    while ((key = it->next (&x))) {
      d->insert (*key, x);
    }
    return d;
  }

  //========================================= bindtab_t ================

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

  //--------------------------------------------------------------------
  
  static nonref_recycler_t<qhash_slot<str, ptr<expr_t> > > _slot_recycler(
                  ok_pub3_recycle_limit_slot, "bindtab_slot");

  static recycler_t<bindtab_t> _bindtab_recycler(
                ok_pub3_recycle_limit_bindtab, "bindtab_t");

  void 
  bindtab_t::finalize() {  
    _bindtab_recycler.recycle(this);
  }

  //--------------------------------------------------------------------
  
  ptr<bindtab_t> 
  bindtab_t::alloc () { 
    return _bindtab_recycler.alloc();
  }

  //============================================= binding_t ============

  binding_t::binding_t (const str &s, ptr<expr_t> x) : _name (s), _expr (x) {}

  //============================================== bindlist_t ==========

  void bindlist_t::insert (binding_t b) { push_back (b); }

  //--------------------------------------------------------------------

  ptr<bindlist_t> bindlist_t::alloc () 
  { return New refcounted<bindlist_t> (plineno ()); }

  //--------------------------------------------------------------------

  ptr<bindtab_t> 
  bindlist_t::keys_only () const
  {
    ptr<bindtab_t> ret = bindtab_t::alloc ();
    for (size_t i = 0; i < size (); i++) {
      ret->insert ((*this)[i].name (), NULL);
    }
    return ret;
  }

  //============================================= bindtab_t ============
  
  ptr<bindtab_t::const_iterator_t> bindtab_t::iter () const 
  { 
    return New refcounted<bindtab_t::const_iterator_t> (*this); 
  }
  
  //====================================================================

  void
  expr_dict_t::v_dump (dumper_t *d) const
  {
    const_iterator_t it (*this);
    const str *key;
    ptr<expr_t> value;
    while ((key = it.next (&value))) {
      s_dump (d, *key, value);
    }
  }

  //--------------------------------------------------------------------

  void
  expr_dict_t::propogate_metadata (ptr<const metadata_t> md)
  {
    iterator_t it (*this);
    ptr<expr_t> value;
    while (it.next (&value)) {
      if (value) value->propogate_metadata (md);
    }
  }

  //--------------------------------------------------------------------

  ptr<expr_dict_t>
  expr_dict_t::safe_copy (ptr<const expr_dict_t> in)
  {
    ptr<expr_dict_t> out;
    if (in) { out = in->copy ()->to_dict (); }
    else { out = expr_dict_t::alloc (); }
    return out;
  }

  //--------------------------------------------------------------------

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

  bool
  expr_dict_t::might_block_uncached () const
  {
    bool mb = false;
    ptr<expr_t> value;
    const_iterator_t it (*this);
    while (it.next (&value) && !mb) {
      if (value && value->might_block ()) { mb = true; }
    }
    return mb;
  }

  //--------------------------------------------------------------------

  ptr<bind_interface_t>
  expr_dict_t::eval_to_bindtab (eval_t *e) const
  {
    ptr<bind_interface_t> out;

    // First see if any keys are static.  Note that this computation
    // will be memoized, so it's fast enough to do a full DFS here.
    // For static objects, make a COW version
    if (is_static ()) {
      out = cow_bindtab_t::alloc (mkref (this));
    } else {
      out = eval_to_val_final (e);
    }
    return out;
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  expr_dict_t::eval_to_val (eval_t *e) const
  { return eval_to_mval (e); }

  //--------------------------------------------------------------------

  ptr<expr_t>
  expr_dict_t::eval_to_mval (eval_t *e) const
  {
    ptr<expr_t> out;

    // First see if any keys are static.  Note that this computation
    // will be memoized, so it's fast enough to do a full DFS here.
    // For static objects, make a COW version
    if (is_static ()) {
      out = expr_cow_t::alloc (mkref (this));
    } else {
      // Otherwise, recurse --- evaluate next layer down...
      out = eval_to_val_final (e);
    }
    return out;
  }

  //--------------------------------------------------------------------

  ptr<expr_dict_t>
  expr_dict_t::eval_to_val_final (eval_t *e) const
  {
    ptr<expr_dict_t> d = expr_dict_t::alloc ();
    const_iterator_t it (*this);
    const str *key;
    ptr<expr_t> value;
    while ((key = it.next (&value))) {
      ptr<expr_t> cx;
      if (value && (cx = value->eval_to_mval (e))) {
	d->insert (*key, cx);
      }
    }
    return d;
  }

  //--------------------------------------------------------------------

  ptr<expr_dict_t>
  expr_dict_t::copy_dict () const
  {
    const_iterator_t it (*this);
    const str *key;
    ptr<expr_t> value;
    ptr<expr_dict_t> ret = expr_dict_t::alloc ();

    while ((key = it.next (&value))) {
      ptr<expr_t> nv;
      if (value) nv = value->copy ();
      ret->insert (*key, nv);
    }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_t> expr_dict_t::deep_copy () const { return copy_dict (); }
  ptr<expr_t> expr_dict_t::cow_copy () const { return copy_dict (); }

  //--------------------------------------------------------------------
  
  static recycler_t<expr_dict_t> _dict_recycler(
            ok_pub3_recycle_limit_dict, "expr_dict_t");

  void 
  expr_dict_t::finalize() {  
    _dict_recycler.recycle(this);
  }

  //--------------------------------------------------------------------

  ptr<expr_dict_t> 
  expr_dict_t::alloc () { 
    return _dict_recycler.alloc();
  }
  
  //--------------------------------------------------------------------

  ptr<expr_dict_t> expr_dict_t::parse_alloc ()
  {
    return _dict_recycler.alloc(plineno());
  }

  //--------------------------------------------------------------------

  void
  expr_dict_t::insert (binding_t p)
  {
    insert (p.name (), p.expr ());
  }

  //--------------------------------------------------------------------

  void expr_dict_t::insert (str k, int64_t i) 
  { insert (k, expr_int_t::alloc (i)); }

  //--------------------------------------------------------------------

  void expr_dict_t::insert (str k, u_int64_t u) 
  { insert (k, expr_uint_t::alloc (u)); }

  //--------------------------------------------------------------------
  
  str
  expr_dict_t::to_str (str_opt_t o) const
  {
    vec<str> v;
    const_iterator_t it (*this);
    const str *key;
    ptr<expr_t> val;
    str ret;

    o.m_quoted = true;
    const char *fmt = o.compact () ? "%s:%s" : "%s : %s";

    while ((key = it.next (&val))) {
      str vs = expr_t::safe_to_str (val, o);
      str ks = json::quote (*key, o);
      strbuf b (fmt, ks.cstr (), vs.cstr ());
      v.push_back (b);
    }
    ret = vec2str (v, '{', '}', o.compact ());
    return ret;
  }

  //--------------------------------------------------------------------

  scalar_obj_t
  expr_dict_t::to_scalar () const
  {
    str_opt_t o (true, false);
    str s = to_str (o);
    return scalar_obj_t (s);
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

  //-----------------------------------------------------------------------

  void expr_dict_t::insert (str k, str v)
  { insert (k, expr_str_t::alloc (v)); }

  //====================================================================

  expr_assignment_t::expr_assignment_t (ptr<expr_t> lhs, ptr<expr_t> rhs,
					lineno_t lineno)
    : expr_t (lineno), _lhs (lhs), _rhs (rhs) {}

  //---------------------------------------------------------------------

  ptr<expr_assignment_t> 
  expr_assignment_t::alloc (ptr<expr_t> l, ptr<expr_t> r)
  { return New refcounted<expr_assignment_t> (l, r, plineno ()); }

  //---------------------------------------------------------------------

  void
  expr_assignment_t::propogate_metadata (ptr<const metadata_t> md)
  {
    if (_lhs) _lhs->propogate_metadata (md);
    if (_rhs) _rhs->propogate_metadata (md);
  }

  //---------------------------------------------------------------------
  
  ptr<mref_t>
  expr_assignment_t::eval_to_ref (eval_t *e) const
  {
    ptr<expr_t> rhs = _rhs->eval_to_mval (e);
    ptr<mref_t> lhs = _lhs->eval_to_ref (e);
    ptr<mref_t> ret = eval_to_ref_final (e, lhs, rhs);

    return ret;
  }

  //---------------------------------------------------------------------

  bool expr_assignment_t::might_block_uncached () const
  { return expr_t::might_block (_lhs, _rhs); }

  //---------------------------------------------------------------------

  ptr<const expr_t>
  expr_assignment_t::eval_to_val (eval_t *e) const
  {
    ptr<mref_t> r = eval_to_ref (e);
    ptr<const expr_t> x;
    if (r) x = r->get_value ();
    return x;
  }

  //---------------------------------------------------------------------

  ptr<mref_t>
  expr_assignment_t::eval_to_ref_final (eval_t *e, ptr<mref_t> lhs,
					ptr<expr_t> rhs) const
  {
    ptr<mref_t> ret;
    ptr<expr_t> v;

    if (!lhs) {
      report_error (e, "error in assignment: LHS evaluates to null");
    } else if (!(lhs->set_value (safe_expr (rhs)))) {
      report_error (e, "error in assignment: invalid LHS");
    } else {
      ret = lhs;
    }

    return ret;
  }
  
  //---------------------------------------------------------------------

  void
  expr_assignment_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "lhs:", _lhs);
    s_dump (d, "rhs:", _rhs);
  }

  //=========================================== expr_cow_t =============

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

  //--------------------------------------------------------------------

  str
  expr_cow_t::type_to_str () const
  {
    str ret;
    ptr<const expr_t> x = const_ptr ();
    if (!x) { ret = expr_t::undef_str; }
    else { ret = x->type_to_str (); }
    return ret;
  }

  //--------------------------------------------------------------------

  const str* expr_cow_t::documentation () const
  {
    ptr<const expr_t> x = const_ptr ();
    if (x) { return x->documentation ();
    } else {
      return NULL;
    }
  }

  //--------------------------------------------------------------------

  str
  expr_cow_t::to_str (str_opt_t o) const 
  {
    str s;
    ptr<const expr_t> x = const_ptr ();
    if (x) { s = x->to_str (o); }
    return s;
  }

  //--------------------------------------------------------------------

#define F(typ,nm)				\
  bool						\
  expr_cow_t::to_##nm (typ *s) const		\
  {						\
    bool ret = false;				\
    ptr<const expr_t> x = const_ptr ();		\
    if (x) { ret = x->to_##nm (s); }		\
    return ret;					\
  }

  F(size_t, len);
  F(int64_t, int);
  F(u_int64_t, uint);
  F(double, double);

#undef F
  
  //--------------------------------------------------------------------

  ptr<rxx>
  expr_cow_t::to_regex (str *errp) const
  {
    ptr<rxx> ret;
    ptr<const expr_t> x = const_ptr ();
    if (x) { ret = x->to_regex (errp); }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<expr_regex_t>
  expr_cow_t::to_regex_obj () 
  {
    ptr<expr_regex_t> ret;
    ptr<expr_t> x = mutable_ptr ();
    if (x) { ret = x->to_regex_obj (); }
    return ret;
  }
  //=====================================================================
  
  // Recycler access functions
  
  recycler_t<bindtab_t> * get_bindtab_recycler() {
    return &_bindtab_recycler;
  }

  recycler_t<expr_int_t> * get_int_recycler() {
    return &_int_recycler;
  }

  recycler_t<expr_dict_t> * get_dict_recycler() {
    return &_dict_recycler;
  }

  nonref_recycler_t<qhash_slot<str, ptr<expr_t> > > * get_slot_recycler() {
    return &_slot_recycler;
  }

  void toggle_recycler_stats(bool enabled) {
    get_int_recycler()->toggle_stats(enabled);
    get_bindtab_recycler()->toggle_stats(enabled);
    get_dict_recycler()->toggle_stats(enabled);
    get_slot_recycler()->toggle_stats(enabled);
  }

};

//=======================================================================


