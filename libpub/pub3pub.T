// -*-c++-*-
#include "pub3pub.h"
#include "pub3file.h"
#include "pub3ast.h"
#include "pub3hilev.h"

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

  //====================================================================

  tamed void
  publish_t::publish (str nm, location_t loc, ptr<bind_interface_t> d, 
		      status_ev_t ev)
  {
    tvars {
      size_t sz;
      xpub_status_t status;
      ptr<file_t> dummy;
    }
    push_include_location (loc);
    sz = env ()->push_locals (d);
    twait { _pub_iface->publish (*this, nm, mkevent (status, dummy)); }
    env ()->pop_to (sz);
    pop_include_location ();
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

    if (_include_stack.size () > ok_pub_max_include_depth) {
      str fn = file->metadata ()->jailed_filename ();
      strbuf msg;
      msg << "maximum include stack depth (" << ok_pub_max_include_depth 
	  << ") exceeded when trying to include file " << fn;
      output_err_stacktrace (msg);
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

  void publish_t::push_include_location (location_t l) 
  { _include_stack.push_back (l); }
  void publish_t::push_metadata (ptr<const metadata_t> md)
  { _metadata_stack.push_back (md); }
  void publish_t::pop_include_location () { _include_stack.pop_back (); }
  void publish_t::pop_metadata () { _metadata_stack.pop_back (); }

  //--------------------------------------------------------------------

  void
  publish_t::output_err_stacktrace (str msg)
  {
    // XXX TODO
  }

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
  publish_t::push_lambda_call (str fn, ptr<bindtab_t> bindings)
  {
    lambda_state_t ret;
    if (_lambda_stack.size () >= ok_pub_max_call_stack_depth) {
      ret._overflow = true;
      output_err ("lambda call stack overflow", _call_location);
    } else {
      ret._binding_stack_size = env ()->push_locals (bindings);
      ret._old_control = push_control ();
      _lambda_stack.push_back (fn);
    }
    return ret;
  }

  //--------------------------------------------------------------------

  ptr<const expr_t>
  publish_t::pop_lambda_call (lambda_state_t state)
  {
    ptr<const expr_t> ret = control ()->rtrn ();
    _lambda_stack.pop_back ();
    restore_control (state._old_control);
    env()->pop_to (state._binding_stack_size);
    return ret;
  }

  //====================================================================

  ptr<output_t>
  output_t::alloc (zbuf *z)
  {
    ptr<output_t> ret;
    if (z) { ret = New refcounted<output_std_t> (z);   }
    else   { ret = New refcounted<output_silent_t> (); }
    return ret;
  }

  //--------------------------------------------------------------------

  bool
  output_t::push_muzzle (bool b)
  {
    bool ret = _muzzle;
    if (b && !_muzzle) { _muzzle = b; }
    return ret;
  }

  //--------------------------------------------------------------------

  void output_t::pop_muzzle (bool b) { _muzzle = b; }

  //====================================================================

  ptr<output_silent_t> output_silent_t::alloc () 
  { return New refcounted<output_silent_t> (); }

  //====================================================================

};


