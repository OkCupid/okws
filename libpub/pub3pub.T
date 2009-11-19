// -*-c++-*-
#include "pub3pub.h"
#include "pub3file.h"
#include "pub3ast.h"
#include "pub3hilev.h"
#include "pub3out.h"

namespace pub3 {

  //======================================= control_t ==================

  bool
  control_t::handle_forloop () 
  {
    bool go = !_break && !_return;
    _break = _continue = false;
    return go;
  }

  //-------------------------------------------------------------------

  bool
  control_t::handle_zone () 
  {
    return !_break && !_return && !_continue;
  }

  //-------------------------------------------------------------------

  void control_t::reset_forloop () { _break = _continue = false; }

  //-------------------------------------------------------------------

  ptr<control_t> control_t::alloc () { return New refcounted<control_t> (); }

  //================================== publish_t =======================

  tamed void
  publish_t::publish (str nm, location_t loc, ptr<bind_interface_t> d, 
		      status_ev_t ev)
  {
    tvars {
      size_t sz;
      xpub_status_t status;
      ptr<file_t> dummy;
    }
    sz = env ()->push_locals (d);
    twait { _pub_iface->publish (*this, nm, mkevent (status, dummy)); }
    env ()->pop_to (sz);
    ev->trigger (status);
  }

  //--------------------------------------------------------------------

  ptr<localizer_t>
  publish_t::localizer () 
  {
    ptr<localizer_t> ret;
    if (!(_opts & int (P_NOLOCALE))) {
      ret = _localizer;
    }
    return ret;
  }

  //--------------------------------------------------------------------

  // For regular file publishing!
  publish_t::publish_t (ptr<bindtab_t> universals, zbuf *z)
    : eval_t (New refcounted<env_t> (universals), output_t::alloc (z)),
      _opts (0),
      _pws (false),
      _control (control_t::alloc ()) {}

  //--------------------------------------------------------------------

  // For configuration file publishing!
  publish_t::publish_t (ptr<bindtab_t> out)
    : eval_t (New refcounted<env_t> (out, out), output_silent_t::alloc ()),
      _opts (0),
      _pws (false),
      _control (control_t::alloc ()) {}

  //--------------------------------------------------------------------

  publish_t::publish_t (ptr<env_t> e, ptr<output_t> o)
    : eval_t (e, o),
      _opts (0),
      _pws (false),
      _control (control_t::alloc ()) {}

  //--------------------------------------------------------------------

  ptr<control_t> publish_t::control () { return _control; }
  void publish_t::restore_control (ptr<control_t> c) { _control = c; }

  //--------------------------------------------------------------------

  ptr<control_t>
  publish_t::push_control ()
  {
    ptr<control_t> c = _control;
    _control = control_t::alloc ();
    return c;
  }

  //--------------------------------------------------------------------

  bool publish_t::pws () const { return _pws; }
  void publish_t::pop_pws (bool b) { _pws = b; }

  //--------------------------------------------------------------------

  bool
  publish_t::push_pws (bool b)
  {
    bool ret = _pws;

    // Only allow, on a push, to go from non-preserve to 
    // preserve mode.
    if (b && !_pws) { _pws = b; }
    return ret;
  }

  //--------------------------------------------------------------------

  tamed void
  publish_t::publish_file (ptr<const file_t> file, status_ev_t ev)
  {
    tvars {
      ptr<const metadata_t> md;
      xpub_status_t status;
      size_t sz;
    }

    if (_stack.size () > ok_pub_max_stack) {
      str fn = file->metadata ()->jailed_filename ();
      strbuf msg;
      msg << "maximum include stack depth (" << ok_pub_max_stack
	  << ") exceeded when trying to include file " << fn;
      output_err_stacktrace (msg, P_ERR_ERROR);
    } else {
      md = file->metadata ();
      push_metadata (md);
      sz = env ()->push_locals (md->to_dict ());
      twait { file->data ()->publish (*this, mkevent (status)); }
      env ()->pop_to (sz);
      pop_metadata ();
    }
    ev->trigger (status);
  }

  //--------------------------------------------------------------------

  void publish_t::push_metadata (ptr<const metadata_t> md) 
  { _stack.push_back (runloc_t (md)); }
  void publish_t::pop_metadata () { _stack.pop_back (); }
  void publish_t::set_lineno (lineno_t lineno) 
  { _stack.back ().set_lineno (lineno); }

  //--------------------------------------------------------------------

  str
  publish_t::set_cwd (str s)
  {
    str ret = _cwd;
    _cwd = s;
    return ret;
  }

  //--------------------------------------------------------------------

  lambda_state_t
  publish_t::push_lambda_call (ptr<const metadata_t> md,
			       str fn, ptr<bindtab_t> bindings)
  {
    lambda_state_t ret;

    _stack.push_back (runloc_t (md, fn));
    ret._binding_stack_size = env ()->push_locals (bindings);
    ret._old_control = push_control ();

    if (_stack.size () > ok_pub_max_stack) {
      ret._overflow = true;
      str msg = "call stack overflow";
      output_err_stacktrace (msg, P_ERR_ERROR);
    }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  publish_t::pop_lambda_call (lambda_state_t state)
  {
    ptr<const expr_t> ret = control ()->rtrn ();
    restore_control (state._old_control);
    env()->pop_to (state._binding_stack_size);
    _stack.pop_back ();
    return ret;
  }

  //--------------------------------------------------------------------

  void publish_t::output (zstr z) { out ()->output (z); }
  void publish_t::output (str z) { out ()->output (z); }

  //--------------------------------------------------------------------

  void publish_t::output_err (str msg, err_type_t typ)
  { out ()->output_err (_stack.back (), msg, typ); }
  void publish_t::output_err_stacktrace (str s, err_type_t t)
  { out ()->output_err (_stack, s, t); }

  //--------------------------------------------------------------------

  ptr<const metadata_t> publish_t::current_metadata () const
  { return _stack.back ().metadata (); }

  //========================================= runloc_t =================

  str
  runloc_t::filename () const
  {
    str r;
    if (_metadata) r = _metadata->jailed_filename (); 
    if (!r) r = "<top-level>";
    return r;
  }

  //--------------------------------------------------------------------

  void
  runloc_t::pub (obj_t &o) const
  {
    o("lineno") = lineno ();
    str f = funcname ();
    if (f) o("function") = f;
    o("filename") = filename ();
  }

  //--------------------------------------------------------------------

  str
  runloc_t::to_str () const
  {
    strbuf b;
    str fn = filename ();
    b << fn << ":" << lineno ();
    if (_func) { b << " (in " << _func << ")"; }
    return b;
  }

  //====================================== loc_stack_t ==================

  obj_list_t 
  loc_stack_t::pub (ssize_t stop) const
  {
    obj_list_t stk;
    ssize_t lim = size () + stop;
    for (ssize_t i = 0 ; i < lim; i++) {
      obj_dict_t d;
      (*this)[i].pub (d);
      stk.push_back (d);
    }
    return stk;
  }

  //====================================================================

};


