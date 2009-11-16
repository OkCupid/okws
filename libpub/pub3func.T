// -*-c++-*-
#include "pub3func.h"
#include "pub3parse.h"


//-----------------------------------------------------------------------
//
// Functions for allocations runtime functions (rfn's)

namespace pub3 {

  //======================================================================

  // XXX copy-pasted from pub3ast.T
  static const location_t &location () 
  { return parser_t::current ()->location (); }

  //======================================== lambda_t =====================

  lambda_t::lambda_t (ptr<identifier_list_t> p, ptr<zone_t> z, location_t loc)
    : _loc (loc), 
      _name ("(anonymous lambda)"),
      _params (p), 
      _body (z) {}

  //-----------------------------------------------------------------------

  ptr<lambda_t> lambda_t::alloc (ptr<identifier_list_t> il, ptr<zone_t> z)
  { return New refcounted<lambda_t> (il, z, location ()); }

  //-----------------------------------------------------------------------

  str 
  lambda_t::to_str () const
  {
    str ls = _loc.to_str ();
    return strbuf ("<%s @ %s>", _name.cstr (), ls.cstr ());
  }

  //-----------------------------------------------------------------------

  bool lambda_t::might_block () const 
  { return (_body && _body->might_block ()); }

  //-----------------------------------------------------------------------
  
  bool
  lambda_t::check_args (publish_t e, args_t a) const
  {
    size_t gs, es;
    gs = a->size ();
    es = _params->size ();
    str s = to_str ();
    bool ret = true;

    if (gs != es) {
      strbuf b ("Wrong number of parameters to call %s "
		"(expected %zd, got %zd)", s.cstr (), es, gs);
      report_error (e, b);
      ret = false;
    } 
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  lambda_t::bind_arg (ptr<bindtab_t> ret, size_t index, ptr<mref_t> r) const
  {
    ptr<expr_t> x;
    if (r) { x = r->get_value (); }
    if (x) { x = expr_null_t::alloc (); }
    ret->insert ((*_params)[index], x);
  }

  //-----------------------------------------------------------------------

  tamed void
  lambda_t::bind_args (publish_t p, fndef_base_t::args_t a, 
		       event<ptr<bindtab_t> >::ref ev) const
  {
    tvars {
      ptr<bindtab_t> ret (New refcounted<bindtab_t> ());
      size_t i;
      ptr<mref_t> r;
    }

    for (i = 0; i < a->size (); i++) {
      twait { (*a)[i]->pub_to_ref (p, mkevent (r)); }
      // XXX -- some warning?
      bind_arg (ret, i, r);
    }
    ev->trigger (ret);
  }


  //-----------------------------------------------------------------------

  ptr<bindtab_t>
  lambda_t::bind_args_nonblock  (publish_t p, args_t args) const
  {
    ptr<bindtab_t> ret = New refcounted<bindtab_t> ();
    for (size_t i = 0; i < args->size (); i++) {
      // XXX -- some warning?
      ptr<mref_t> r = (*args)[i]->eval_to_ref (p);
      bind_arg (ret, i, r);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t> 
  lambda_t::eval_to_val (publish_t p, args_t args) const
  {
    ptr<const expr_t> ret;
    ptr<control_t> old_ctrl;
    ptr<bindtab_t> bindings;
    size_t sz;

    if (check_args (p, args)) {
      old_ctrl = p.push_control ();
      bindings = bind_args_nonblock (p, args);
      sz = p.env ()->push_locals (bindings);

      // XXX check stack depth
      _body->publish_nonblock (p);
      p.env ()->pop_to (sz);

      ret = p.control ()->rtrn ();
      p.restore_control (old_ctrl);
    }
    return ret;
  }

  //======================================== fndef_t ======================

  bool
  fndef_t::publish_nonblock (publish_t p) const
  {
    p.env ()->add_global_binding (_name, _lambda);
    return true;
  }

  //============================================= call_t  ================

  ptr<call_t> call_t::alloc (ptr<expr_t> x, ptr<expr_list_t> l)
  { return New refcounted<call_t> (x, l, plineno ()); }

  //----------------------------------------------------------------------

  void call_t::unshift_argument (ptr<expr_t> x)
  { _arglist->push_front (x); }

  //----------------------------------------------------------------------

  tamed void
  call_t::pub_to_val (publish_t p, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      ptr<const fndef_base_t> fnd;
    }

    twait { pub_prepare (p, mkevent (fnd)); }
    if (!fnd) { /* noop */ }
    else if (fnd->might_block () || _arglist->might_block ()) {
      twait { fnd->pub_to_val (p, _arglist, mkevent (ret)); }
    } else {
      ret = fnd->eval_to_val (p, _arglist);
    }
    ev->trigger (ret);
  }

  //----------------------------------------------------------------------

  tamed void
  call_t::pub_to_ref (publish_t p, mrev_t ev) const
  {
    tvars {
      ptr<const expr_t> x;
      ptr<expr_t> copy;
      ptr<mref_t> ret;
    }
    twait { pub_to_val (p, mkevent (x)); }
    if (x) copy = x->copy ();
    ret = const_mref_t::alloc (copy);
    ev->trigger (ret);
  }

  //----------------------------------------------------------------------

  tamed void
  call_t::pub_prepare (publish_t p, 
		       event<ptr<const fndef_base_t> >::ref ev) const
  {
    tvars {
      ptr<const expr_t> f;
      ptr<const fndef_base_t> fnd;
    }

    p.update_location (*this);

    if (_fn->might_block ()) { twait { _fn->pub_to_val (p, mkevent (f)); } }
    else { f = _fn->eval_to_val (p); }
    
    if (!f) { 
      report_error (p, "cannot resolve function"); 
    } else if (!(fnd = f->to_fndef ())) {
      report_error (p, "expected a function; got something else");
    }
    ev->trigger (fnd);
  }

  //======================================================================

};

//
// end RFNs
//-----------------------------------------------------------------------
