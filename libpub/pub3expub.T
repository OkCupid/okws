// -*-c++-*-
#include "pub3pub.h"
#include "pub3expr.h"
#include "pub3func.h"

namespace pub3 {

  //=======================================================================

  void
  expr_t::pub_to_val (publish_t pub, cxev_t ev, ptr<closure_t> dummy) const
  {
    ev->trigger (eval_to_val (pub));
  }

  //-----------------------------------------------------------------------

  void
  expr_t::pub_to_ref (publish_t pub, mrev_t ev, ptr<closure_t> dummy) const
  {
    ev->trigger (eval_to_ref (pub));
  }

  //-----------------------------------------------------------------------

  tamed void 
  expr_t::pub_as_bool (publish_t pub, evb_t ev) const
  {
    tvars {
      bool ret (false);
      bool l;
      ptr<const expr_t> x;
    }

    l = pub.set_silent (true);
    twait { pub_to_val (pub, mkevent (x)); }
    pub.set_silent (l);
    if (x) ret = x->to_bool ();
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_as_str (publish_t pub, evs_t ev) const
  {
    tvars {
      ptr<const expr_t> x;
      str ret;
    }
    twait { pub_to_val (pub, mkevent (x)); }
    if (x) {
      bool q = !x->is_str ();
      ret = x->to_str (q);
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  tamed void
  expr_t::pub_as_null (publish_t pub, evb_t ev) const
  {
    tvars {
      bool ret (true);
      bool l;
      ptr<const expr_t> x;
    }
    l = pub.set_silent (true);
    twait { pub_to_val (pub, mkevent (x)); }
    pub.set_silent (l);
    if (x) ret = x->is_null ();
    ev->trigger (ret);
  }

  //=======================================================================

  tamed void
  expr_logical_t::pub_to_val (publish_t p, cxev_t ev) const
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
  expr_OR_t::pub_logical (publish_t p, evb_t ev) const
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
  expr_AND_t::pub_logical (publish_t p, evb_t ev) const
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
  expr_NOT_t::pub_logical (publish_t p, evb_t ev) const
  {
    tvars {
      bool res (false);
    }
    if (_e) { twait { pub_as_bool (p, mkevent (res)); } }
    ev->trigger (!res);
  }

  //=======================================================================

  tamed void
  expr_EQ_t::pub_logical (publish_t p, evb_t ev) const
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
  expr_relation_t::pub_logical (publish_t p, evb_t ev) const
  {
    tvars {
      bool ret (false);
      ptr<const expr_t> l, r;
    }
    if (_l) { twait { _l->pub_to_val (p, mkevent (l)); } }
    if (_r) { twait { _r->pub_to_val (p, mkevent (r)); } }
    ret = eval_final (p, l, r);
    ev->trigger (ret);
  }

  //=======================================================================

  tamed void
  expr_binaryop_t::pub_to_val (publish_t pub, cxev_t ev)
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
  expr_varref_or_rfn_t::pub_to_val (publish_t e, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> r, v;
      ptr<const proc_call_t> fnd;
      ptr<const expr_t> rfn;
      bool make_silent;
      bool old_silent;
    }

    make_silent = _arglist;
    old_silent = e.set_silent (make_silent);
    twait { expr_varref_t::pub_to_val (e, mkevent (v)); }
    e.set_silent (old_silent);

    if (!_arglist) { r = v; }
    else if (v && (fnd = v->to_proc_call ())) {
      twait { fnd->pub_to_val (e, _arglist, mkevent (r)); }
    } else if ((rfn = get_rfn ())) {
      twait { rfn->pub_to_val (e, mkevent (r)); }
    }
    ev->trigger (r);
  }
  
  //-----------------------------------------------------------------------

  tamed void
  expr_varref_or_rfn_t::pub_to_ref (publish_t e, mrev_t ev) const
  {
    tvars {
      ptr<mref_t> r, v;
      ptr<expr_t> x;
      ptr<const proc_call_t> fnd;
      ptr<const expr_t> rfn;
      bool make_silent, old_silent;
    }

    make_silent = _arglist;
    old_silent = e.set_silent (make_silent);
    twait { expr_varref_t::pub_to_ref (e, mkevent (v)); }
    e.set_silent (old_silent);

    if (!_arglist) { r = v; }
    else if (v && (x = v->get_value ()) && (fnd = x->to_proc_call ())) { 
      twait { fnd->pub_to_ref (e, _arglist, mkevent (r)); }
    } else if ((rfn = get_rfn ())) {
      twait { rfn->pub_to_ref (e, mkevent (r)); }
    }
    ev->trigger (r);
  }
  
  //=======================================================================

  tamed void
  expr_vecref_t::pub_to_val (publish_t p, cxev_t ev) const
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
  expr_vecref_t::pub_to_ref (publish_t p, mrev_t ev) const
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
  expr_dictref_t::pub_to_ref (publish_t pub, mrev_t ev) const
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
  expr_dictref_t::pub_to_val (publish_t pub, cxev_t ev) const
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
  expr_assignment_t::pub_to_ref (publish_t pub, mrev_t ev) const
  {
    tvars {
      ptr<mref_t> lhs, rhs, ret;
    }
    twait { _rhs->pub_to_ref (pub, mkevent (rhs)); }
    twait { _lhs->pub_to_ref (pub, mkevent (lhs)); }
    ret = eval_to_ref_final (pub, lhs, rhs);
    ev->trigger (ret);
  }

  //=======================================================================

};
