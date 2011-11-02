// -*-c++-*-

#include "pub3eval.h"
#include "pub3expr.h"
#include "pub3func.h"

namespace pub3 {

  //====================================== expr_t ==========================

  void
  expr_t::pub_to_val (eval_t *pub, cxev_t ev, ptr<closure_t> dummy) const
  {
    ev->trigger (eval_to_val (pub));
  }

  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_to_ref (eval_t *pub, mrev_t ev) const
  {
    tvars {
      ptr<expr_t> x;
      ptr<const expr_t> v;
      ptr<const_mref_t> ret;
    }
    twait { pub_to_val (pub, mkevent (v)); }
    if (v) { x = v->copy (); }
    ret = const_mref_t::alloc (x);
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  tamed void 
  expr_t::pub_as_bool (eval_t *pub, evb_t ev) const
  {
    tvars {
      bool ret (false);
      bool l;
      ptr<const expr_t> x;
    }

    l = pub->set_silent (true);
    twait { pub_to_val (pub, mkevent (x)); }
    pub->set_silent (l);
    if (x) ret = x->to_bool ();
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_to_bindtab (eval_t *pub, biev_t ev) const
  {
    tvars {
      ptr<const expr_dict_t> d;
      ptr<bind_interface_t> bi;
    }
    twait { pub_as_dict (pub, mkevent (d)); }
    if (d) {
      bi = cow_bindtab_t::alloc (d);
    }
    ev->trigger (bi);
  }

  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_as_scalar (eval_t *pub, event<scalar_obj_t>::ref ev) const
  {
    tvars {
      scalar_obj_t ret;
      ptr<const expr_t> x;
    }
    twait { pub_to_val (pub, mkevent (x)); }
    if (x) { ret = x->to_scalar (); }
    ev->trigger (ret);
  }

  //---------------------------------------------------------------------

  tamed void
  expr_t::pub_as_str (eval_t *pub, evs_t ev) const
  {
    tvars {
      ptr<const expr_t> x;
      str ret;
    }
    twait { pub_to_val (pub, mkevent (x)); }
    if (x) {
      bool q = !x->is_str ();
      str_opt_t o (q, pub->utf8_json ());
      ret = x->to_str (o);
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_as_dict (eval_t *pub, cdev_t ev) const
  {
    tvars {
      ptr<const expr_t> x;
      ptr<const expr_dict_t> d;
    }
    twait { pub_to_val (pub, mkevent (x)); }
    if (x) { d = x->to_dict (); }
    ev->trigger (d);
  }
      
  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_as_null (eval_t *pub, evb_t ev) const
  {
    tvars {
      bool ret (true);
      bool l;
      ptr<const expr_t> x;
    }
    l = pub->set_silent (true);
    twait { pub_to_val (pub, mkevent (x)); }
    pub->set_silent (l);
    if (x) ret = x->is_null ();
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_to_mval (eval_t *p, xev_t ev) const
  { 
    tvars {
      ptr<const expr_t> x;
      ptr<expr_t> ret;
    }
    twait { pub_to_val (p, mkevent (x)); }
    if (x) { ret = x->copy (); }
    ev->trigger (ret);
  }

  //================================== expr_logical_t =====================

  tamed void
  expr_logical_t::pub_to_val (eval_t *p, cxev_t ev) const
  {
    tvars {
      bool res;
    }
    if (!might_block ()) { res = eval_logical (p); }
    else { twait { pub_logical (p, mkevent (res)); } }
    ev->trigger (expr_bool_t::alloc (res));
  }

  //=======================================================================

  tamed void
  expr_OR_t::pub_logical (eval_t *p, evb_t ev) const
  {
    tvars {
      bool res;
      bool b1 (false), b2 (false);
    }
    if (_t1) { twait { _t1->pub_as_bool (p, mkevent (b1)); } }
    if (!b1 && _t2) { twait { _t2->pub_as_bool (p, mkevent (b2)); } }
    res = b1 || b2;
    ev->trigger (res);
  }

  //=======================================================================

  tamed void
  expr_AND_t::pub_logical (eval_t *p, evb_t ev) const
  {
    tvars {
      bool res;
      bool b1 (false), b2 (false);
    }
    if (_f1) { twait { _f1->pub_as_bool (p, mkevent (b1)); } }
    if (b1 && _f2) { twait { _f2->pub_as_bool (p, mkevent (b2)); } }
    res = b1 && b2;
    ev->trigger (res);
  }

  //=======================================================================

  tamed void
  expr_NOT_t::pub_logical (eval_t *p, evb_t ev) const
  {
    tvars {
      bool res (false);
    }
    if (_e) { twait { _e->pub_as_bool (p, mkevent (res)); } }
    ev->trigger (!res);
  }

  //=======================================================================

  tamed void
  expr_EQ_t::pub_logical (eval_t *p, evb_t ev) const
  {
    tvars {
      bool ret (false);
      ptr<const expr_t> x1, x2;
    }
    if (_o1) { twait { _o1->pub_to_val (p, mkevent (x1)); } }
    if (_o2) { twait { _o2->pub_to_val (p, mkevent (x2)); } }
    ret = eval_final (x1, x2);
    ev->trigger (ret);
  }

  //=======================================================================

  tamed void
  expr_relation_t::pub_logical (eval_t *p, evb_t ev) const
  {
    tvars {
      bool ret (false);
      ptr<const expr_t> l, r;
    }
    if (_l) { twait { _l->pub_to_val (p, mkevent (l)); } }
    if (_r) { twait { _r->pub_to_val (p, mkevent (r)); } }
    ret = eval_final (p, l, r, _op, this);
    ev->trigger (ret);
  }

  //=======================================================================

  tamed void
  expr_binaryop_t::pub_to_val (eval_t *pub, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> e1, e2;
      ptr<const expr_t> res;
    }
    
    if (_o1) { twait { _o1->pub_to_val (pub, mkevent (e1)); } }
    if (_o2) { twait { _o2->pub_to_val (pub, mkevent (e2)); } }

    res = eval_final (pub, e1, e2);
    ev->trigger (res);
  }

  //=======================================================================

  tamed void
  expr_vecref_t::pub_to_val (eval_t *p, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> c, i, r;
    }
    twait { _vec->pub_to_val (p, mkevent (c)); }
    twait { _index->pub_to_val (p, mkevent (i)); }
    r = eval_to_val_final (p, c, i);
    ev->trigger (r);
  }

  //--------------------------------------------------------------------

  tamed void
  expr_vecref_t::pub_to_ref (eval_t *p, mrev_t ev) const
  {
    tvars {
      ptr<mref_t> cr;
      ptr<const expr_t> i;
      ptr<mref_t> r;
    }
    twait { _vec->pub_to_ref (p, mkevent (cr)); }
    twait { _index->pub_to_val (p, mkevent (i)); }
    r = eval_to_ref_final (p, cr, i);
    ev->trigger (r);
  }

  //=======================================================================

  tamed void
  expr_dictref_t::pub_to_ref (eval_t *pub, mrev_t ev) const
  {
    tvars {
      ptr<mref_t> v;
      ptr<mref_t> r;
    }
    twait { _dict->pub_to_ref (pub, mkevent (v)); }
    r = eval_to_ref_final (pub, v);
    ev->trigger (r);
  }

  //--------------------------------------------------------------------

  tamed void
  expr_dictref_t::pub_to_val (eval_t *pub, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> d;
      ptr<const expr_t> r;
    }
    twait { _dict->pub_to_val (pub, mkevent (d)); }
    r = eval_to_val_final (pub, d);
    ev->trigger (r);
  }

  //=======================================================================

  tamed void 
  expr_assignment_t::pub_to_ref (eval_t *pub, mrev_t ev) const
  {
    tvars {
      ptr<mref_t> lhs, ret;
      ptr<expr_t> rhs;
    }
    
    twait { _rhs->pub_to_mval (pub, mkevent (rhs)); }
    twait { _lhs->pub_to_ref (pub, mkevent (lhs)); }
    ret = eval_to_ref_final (pub, lhs, rhs);
    ev->trigger (ret);
  }

  //--------------------------------------------------------------------

  tamed void
  expr_assignment_t::pub_to_val (eval_t *pub, cxev_t ev) const
  {
    tvars {
      ptr<mref_t> r;
      ptr<const expr_t> x;
    }
    twait { pub_to_ref (pub, mkevent (r)); }
    if (r) x = r->get_value ();
    ev->trigger (x);
  }

  //=======================================================================

  tamed void
  expr_dict_t::pub_to_mval (eval_t *p, xev_t ev) const
  {
    tvars {
      ptr<expr_t> out;
      ptr<expr_dict_t> d;
    }

    // First see if any keys are static.  Note that this computation
    // will be memoized, so it's fast enough to do a full DFS here.
    // For static objects, make a COW version
    if (is_static ()) {
      out = expr_cow_t::alloc (mkref (this));
    } else {
      twait { pub_to_val_final (p, mkevent (d)); }
      out = d;
    }
    ev->trigger (out);
  }

  //---------------------------------------------------------------------

  tamed void
  expr_dict_t::pub_to_val (eval_t *p, cxev_t ev) const
  {
    tvars { ptr<expr_t> x; }
    twait { pub_to_mval (p, mkevent (x)); }
    ev->trigger (x);
  }

  //---------------------------------------------------------------------

  tamed void
  expr_dict_t::pub_to_bindtab (eval_t *p, biev_t ev) const
  {
    tvars {
      ptr<bind_interface_t> out;
      ptr<expr_dict_t> d;
    }
    if (is_static ()) {
      out = cow_bindtab_t::alloc (mkref (this));
    } else {
      twait { pub_to_val_final (p, mkevent (d)); }
      out = d;
    }
    ev->trigger (out);
  }

  //---------------------------------------------------------------------

  tamed void
  expr_dict_t::pub_to_val_final (eval_t *p, xdev_t ev) const
  {
    tvars {
      ptr<expr_dict_t> d;
      ptr<expr_t> value, x;
      expr_dict_t::const_iterator_t it (*_self);
      const str *key;
    }
    d = expr_dict_t::alloc();
    while ((key = it.next (&value))) {
      x = NULL;
      if (value) { twait { value->pub_to_mval (p, mkevent (x)); } }
      if (x) { d->insert (*key, x); }
    }
    ev->trigger (d);
  }

  //=======================================================================

  tamed void
  expr_list_t::pub_to_val (eval_t *p, cxev_t ev) const
  {
    tvars { ptr<expr_t> x; }
    twait { pub_to_mval (p, mkevent (x)); }
    ev->trigger (x);
  }

  //----------------------------------------------------------------------

  tamed void
  expr_list_t::pub_to_mval (eval_t *p, xev_t ev) const
  {
    tvars {
      ptr<expr_t> out;
      size_t l;
      ptr<expr_list_t> nl;
      size_t i;
      ptr<expr_t> value, nv;
    }
    if (is_static ()) {
      out = expr_cow_t::alloc (mkref (this));
    } else {
      l = vec_base_t::size ();
      nl = New refcounted<expr_list_t> ();
      for (i = 0; i < l; i++) {
	value = (*this)[i];
	nv = NULL;
	if (value) { twait { value->pub_to_mval (p, mkevent (nv)); } }
	if (!nv) { nv = expr_null_t::alloc (); }
	nl->push_back (nv);
      }
      out = nl;
    }
    ev->trigger (out);
  }

  //========================================= expr_shell_str_t ============

  tamed void
  expr_shell_str_t::pub_to_val (eval_t *p, cxev_t ev) const
  {
    tvars {
      ptr<expr_t> out;
      vec<str> hold;
      strbuf b;
      size_t sz;
      bool ok (true);
      size_t i;
      ptr<const expr_t> x;
      str s;
      str_opt_t sot (false, p->utf8_json ());
    }

    if (_els) {
      sz = _els->size ();
      for (i = 0; ok && i < sz; i++) {
	s = NULL;
	x = (*_els)[i];
	if (x) { twait { x->pub_to_val (p, mkevent (x)); } }
	if (x) { s = x->to_str (sot); }
	if (s) { 
	  hold.push_back (s);
	  b << s; 
	}
      }
      if (ok) { out = expr_str_t::alloc (b); }
    }
    if (!out) { out = expr_null_t::alloc (); }
    ev->trigger (out);
  }

  //=========================== expr_ref_t ===============================

  tamed void
  expr_ref_t::pub_to_mval (eval_t *p, xev_t ev) const
  {
    tvars {
      ptr<mref_t> r;
      ptr<expr_t> ret;
    }
    twait { pub_to_ref (p, mkevent (r)); }
    if (r) { ret = r->get_value (); }
    ev->trigger (ret);
  }

  //======================================================================

};
