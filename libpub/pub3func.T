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

  //=================================================== callable_t ========

  void 
  callable_t::pub_to_val (publish_t *p, args_t args, cxev_t ev, 
			  ptr<closure_t> dummy) const 
  { ev->trigger (eval_to_val (p, args)); }

  //-----------------------------------------------------------------------

  tamed void
  callable_t::pub_to_ref (publish_t *p, callable_t::args_t args, 
			  mrev_t ev) const
  {
    tvars {
      ptr<const expr_t> cx;
      ptr<expr_t> copy;
      ptr<mref_t> ret;
    }
    twait { pub_to_val (p, args, mkevent (cx)); }
    if (cx) { copy = cx->copy (); }
    if (copy) { ret = const_mref_t::alloc (copy); }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  ptr<expr_t>
  callable_t::eval_to_mval (publish_t *p, args_t args) const
  {
    ptr<expr_t> ret;
    ptr<const expr_t> x = eval_to_val (p, args);
    if (x) { ret = x->copy (); }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  callable_t::pub_to_mval (publish_t *p, callable_t::args_t args,
			   xev_t ev) const
  {
    tvars {
      ptr<const expr_t> cx;
      ptr<expr_t> ret;
    }
    twait { pub_to_val (p, args, mkevent (cx)); }
    if (cx) { ret = cx->copy (); }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  ptr<mref_t>
  callable_t::eval_to_ref (publish_t *p, args_t args) const
  {
    ptr<expr_t> copy;
    ptr<mref_t> ret;
    ptr<const expr_t> cx = eval_to_val (p, args);
    if (cx) { copy = cx->copy (); }
    if (copy) { ret = const_mref_t::alloc (copy); }
    return ret;
  }

  //======================================== lambda_t =====================

  lambda_t::lambda_t (ptr<identifier_list_t> p, ptr<zone_t> z, location_t loc)
    : _loc (loc), 
      _name ("anonymous-lambda"),
      _params (p), 
      _body (z) {}

  //-----------------------------------------------------------------------

  void lambda_t::propogate_metadata (ptr<const metadata_t> md)
  { _loc.set_filename (md->jailed_filename ()); }

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
  lambda_t::check_args (publish_t *p, args_t a) const
  {
    size_t gs, es;
    gs = a->size ();
    es = _params->size ();
    str s = to_str ();
    bool ret = true;

    if (gs != es) {
      strbuf b ("Wrong number of parameters to call %s "
		"(expected %zd, got %zd)", s.cstr (), es, gs);
      report_error (p, b);
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
    if (!x) { x = expr_null_t::alloc (); }
    ret->insert ((*_params)[index], x);
  }

  //-----------------------------------------------------------------------

  tamed void
  lambda_t::bind_args (publish_t *p, callable_t::args_t a, 
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
  lambda_t::bind_args_nonblock (publish_t *p, args_t args) const
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
  lambda_t::eval_to_val (publish_t *p, args_t args) const
  {
    ptr<const expr_t> ret;
    ptr<control_t> old_ctrl;
    ptr<bindtab_t> bindings;
    lambda_state_t state;

    if (check_args (p, args)) {
      bindings = bind_args_nonblock (p, args);
      state = p->push_lambda_call (_md, _name, bindings, closure_stack ());
      if (state.is_ok ()) { 
	_body->publish_nonblock (p);
      }
      ret = p->pop_lambda_call (state);
    }
    return expr_t::safe_expr (ret);
  }

  //-----------------------------------------------------------------------

  tamed void
  lambda_t::pub_to_val (publish_t *p, callable_t::args_t args, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      ptr<bindtab_t> bindings;
      lambda_state_t state;
      status_t status;
    }

    if (check_args (p, args)) {
      twait { bind_args (p, args, mkevent (bindings)); }
      state = p->push_lambda_call (_md, _name, bindings, closure_stack ());
      if (state.is_ok ()) {
	twait { _body->publish (p, mkevent (status)); }
      }
      ret = p->pop_lambda_call (state);
    }
    ev->trigger (expr_t::safe_expr (ret));
  }

  //-----------------------------------------------------------------------

  ptr<lambda_t>
  lambda_t::copy (eval_t *e) const
  {
    ptr<lambda_t> l = New refcounted<closed_lambda_t> (*this, e);
    return l;
  }
  
  //-----------------------------------------------------------------------

  ptr<const expr_t> lambda_t::eval_to_val (eval_t *e) const { return copy (e); }

  //-----------------------------------------------------------------------

  ptr<mref_t> lambda_t::eval_to_ref (eval_t *e) const
  { return const_mref_t::alloc (copy (e)); }

  //======================================== closed_lambda_t ==============

  closed_lambda_t::closed_lambda_t (const lambda_t &l, eval_t *e)
    : lambda_t (l)
  { 
    _md = e->current_metadata (); 
    e->env ()->capture_closure (&_stack); 
  }

  //======================================== fndef_t ======================

  status_t
  fndef_t::v_publish_nonblock (publish_t *p) const
  {
    status_t ret (XPUB_STATUS_OK);
    p->env ()->add_global_binding (_name, _lambda->copy (p));
    return ret;
  }

  //-----------------------------------------------------------------------

  void fndef_t::propogate_metadata (ptr<const metadata_t> md)
  { if (_lambda) _lambda->propogate_metadata (md); }

  //-----------------------------------------------------------------------

  ptr<fndef_t> fndef_t::alloc (str nm, ptr<lambda_t> d)
  { return New refcounted<fndef_t> (nm, d, location ()); }

  //-----------------------------------------------------------------------

  void
  fndef_t::v_dump (dumper_t *d) const
  {
    d->dump (strbuf ("name: %s", _name.cstr ()), true);
    s_dump (d, "lambda:", _lambda);
  }

  //============================================= call_t  ================

  ptr<call_t> call_t::alloc (ptr<expr_t> x, ptr<expr_list_t> l)
  { return New refcounted<call_t> (x, l, plineno ()); }

  //----------------------------------------------------------------------

  void
  call_t::propogate_metadata (ptr<const metadata_t> md)
  {
    if (_arglist) _arglist->propogate_metadata (md);
    if (_fn) _fn->propogate_metadata (md);
  }

  //----------------------------------------------------------------------

  void call_t::unshift_argument (ptr<expr_t> x)
  { _arglist->push_front (x); }

  //----------------------------------------------------------------------

  tamed void
  call_t::pub_to_val (publish_t *p, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      ptr<const callable_t> fnd;
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
  call_t::pub_to_ref (publish_t *p, mrev_t ev) const
  {
    tvars {
      ptr<mref_t> ret;
      ptr<const callable_t> fnd;
    }
    twait { pub_prepare (p, mkevent (fnd)); }
    if (!fnd) { /* noop */ }
    else if (fnd->might_block () || _arglist->might_block ()) {
      twait { fnd->pub_to_ref (p, _arglist, mkevent (ret)); }
    } else {
      ret = fnd->eval_to_ref (p, _arglist);
    }
    ev->trigger (ret);
  }

  //----------------------------------------------------------------------

  tamed void
  call_t::pub_to_mval (publish_t *p, xev_t ev) const
  {
    tvars {
      ptr<expr_t> ret;
      ptr<const callable_t> fnd;
    }
    twait { pub_prepare (p, mkevent (fnd)); }
    if (!fnd) { /* noop */ }
    else if (fnd->might_block () || _arglist->might_block ()) {
      twait { fnd->pub_to_mval (p, _arglist, mkevent (ret)); }
    } else {
      ret = fnd->eval_to_mval (p, _arglist);
    }
    ev->trigger (ret);
  }

  //----------------------------------------------------------------------

  ptr<call_t> call_t::alloc (ptr<expr_t> x, lineno_t l) 
  { return New refcounted<call_t> (x, expr_list_t::alloc (), l); }

  //----------------------------------------------------------------------

  tamed void
  call_t::pub_prepare (publish_t *p, 
		       event<ptr<const callable_t> >::ref ev) const
  {
    tvars {
      ptr<const expr_t> f;
      ptr<const callable_t> fnd;
    }

    p->set_lineno (_lineno);

    if (_fn->might_block ()) { twait { _fn->pub_to_val (p, mkevent (f)); } }
    else { f = _fn->eval_to_val (p); }

    if (!f) { 
      report_error (p, "cannot resolve function"); 
    } else if (!(fnd = f->to_callable ())) {
      report_error (p, "expected a function; got something else");
    }
    ev->trigger (fnd);
  }

  //----------------------------------------------------------------------

  void
  call_t::v_dump (dumper_t *d) const
  {
    s_dump (d, "args:", _arglist);
    s_dump (d, "func:", _fn);
  }

  //======================================================================

};

//
// end RFNs
//-----------------------------------------------------------------------
