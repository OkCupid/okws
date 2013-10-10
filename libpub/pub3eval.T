// -*-c++-*-
#include "pub3eval.h"
#include "pub3out.h"
#include "pub3hilev.h"
#include "pub3profiler.h"

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
    return !_break && !_return && !_continue && !_exit;
  }

  //-------------------------------------------------------------------

  void control_t::reset_forloop () { _break = _continue = false; }

  //-------------------------------------------------------------------

  void control_t::reset_file () { _exit = false; }

  //-------------------------------------------------------------------

  ptr<control_t> control_t::alloc () { return New refcounted<control_t> (); }

  //================================================ eval_t ================

  eval_t::eval_t (ptr<env_t> e, ptr<output_t> o, opts_t opts)
    : _env (e), 
      _output (o), 
      _loud (true), 
      _silent (false), 
      _opts (opts),
      _control (control_t::alloc ()) {}

  //-----------------------------------------------------------------------

  // For regular file publishing!
  eval_t::eval_t (ptr<bindtab_t> unis, zbuf *z, opts_t o)
    : _env (New refcounted<env_t> (unis)),
      _output (output_t::alloc (z, o)),
      _loud (true),
      _silent (false), 
      _opts (o),
      _control (control_t::alloc ()) {}

  //--------------------------------------------------------------------

  // For configuration file publishing!
  eval_t::eval_t (ptr<bindtab_t> unis, ptr<bindtab_t> out, opts_t o)
    : _env (New refcounted<env_t> (unis, out)), 
      _output (output_silent_t::alloc (o)),
      _loud (true),
      _silent (false),
      _opts (o),
      _control (control_t::alloc ()) {}

  //--------------------------------------------------------------------

  void eval_t::clone_env () { _env = _env->clone (); }

  //--------------------------------------------------------------------

  void 
  eval_t::silence_output () 
  {
    _output = New refcounted<output_silent_t> (_output->get_opts ());
  }

  //--------------------------------------------------------------------

  ptr<eval_t>
  eval_t::clone () const
  {
    ptr<eval_t> ret = New refcounted<eval_t> (*this);
    ret->clone_env ();
    ret->silence_output ();
    return ret;
  }

  //--------------------------------------------------------------------

  eval_t::~eval_t () 
  {
    for (size_t i = 0; i < _to_clear.size (); i++) {
      _to_clear[i]->cycle_clear ();
    }
    _to_clear.clear ();
  }

  //--------------------------------------------------------------------

  ptr<control_t> eval_t::control () { return _control; }
  void eval_t::restore_control (ptr<control_t> c) { _control = c; }

  //--------------------------------------------------------------------

  ptr<control_t>
  eval_t::push_control ()
  {
    ptr<control_t> c = _control;
    _control = control_t::alloc ();
    return c;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  eval_t::lookup_val (const str &nm) const
  {
    return _env->lookup_val (nm);
  }

  //-----------------------------------------------------------------------

  ptr<mref_t>
  eval_t::lookup_ref (const str &nm) const
  {
    return _env->lookup_ref (nm);
  }

  //-----------------------------------------------------------------------

  void
  eval_t::add_err_obj (str n)
  {
    env ()->add_global_binding (n, out ()->err_obj ());
  }

  //-----------------------------------------------------------------------

  bool eval_t::set_active (bool b) { return _stack.set_active (b); }

  //-----------------------------------------------------------------------
  
  bool
  eval_t::set_loud (bool b)
  {
    bool c = _loud;
    _loud = b;
    return c;
  }
  
  //-----------------------------------------------------------------------
  
  bool
  eval_t::set_silent (bool b)
  {
    bool c = _silent;
    _silent = b;
    return c;
  }

  //-----------------------------------------------------------------------

  bool eval_t::push_muzzle (bool b) { return _output->push_muzzle (b); }
  void eval_t::pop_muzzle (bool b) { _output->pop_muzzle (b); }

  //-----------------------------------------------------------------------

  void
  eval_t::report_error (str msg, lineno_t ln)
  {
    if (ln) set_lineno (ln);
    output_err (msg, P_ERR_EVAL);
  }

  //-----------------------------------------------------------------------

  void eval_t::report_error (str msg, location_t l) 
  { report_error (msg, l._lineno); }

  //-----------------------------------------------------------------------

  ptr<output_t>
  eval_t::set_output (ptr<output_t> no)
  {
    ptr<output_t> ret = _output;
    _output = no;
    return ret;
  }

  //-----------------------------------------------------------------------

  lambda_state_t
  eval_t::push_lambda_call (ptr<const metadata_t> md,
			    str fn, ptr<bindtab_t> bindings,
			    const env_t::stack_t *cls_stk)
  {
    lambda_state_t ret;

    _stack.push_back (runloc_t (md, fn));
    ret._binding_stack_size = env ()->push_lambda (bindings, cls_stk);
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
  eval_t::pop_lambda_call (lambda_state_t state)
  {
    ptr<const expr_t> ret = control ()->rtrn ();
    restore_control (state._old_control);
    env()->pop_to (state._binding_stack_size);
    _stack.pop_back ();
    return ret;
  }

  //--------------------------------------------------------------------

  void eval_t::output_err (str msg, err_type_t typ)
  { 
    if (_stack.size ()) {
      out ()->output_err (_stack.back (), msg, typ); 
    } else {
      out ()->output_err (msg, typ);
    }
    if (_opts & P_EXIT_ON_ERROR) {
      control()->set_exit(true);
    }
  }

  //--------------------------------------------------------------------

  void eval_t::output_err_stacktrace (str s, err_type_t t)
  {
    out ()->output_err (_stack, s, t);
    if (_opts & P_EXIT_ON_ERROR) {
      control()->set_exit(true);
    }
  }

  //--------------------------------------------------------------------

  ptr<const metadata_t> eval_t::current_metadata () const
  { return _stack.back ().metadata (); }
  void eval_t::push_metadata (ptr<const metadata_t> md) 
  { _stack.push_back (runloc_t (md)); }
  void eval_t::pop_metadata () { _stack.pop_back (); }
  void eval_t::set_lineno (lineno_t lineno) 
  { _stack.back ().set_lineno (lineno); }

  //-----------------------------------------------------------------------

  tamed void
  eval_t::publish (str nm, location_t loc, ptr<bind_interface_t> d, 
		      status_ev_t ev)
  {
    tvars {
      size_t sz;
      xpub_status_t status;
      ptr<file_t> dummy;
      env_t::layer_type_t lt;
    }

    lt = (_opts & P_STRICT_INCLUDE_SCOPING) ? 
      env_t::LAYER_LOCALS_BARRIER : 
      env_t::LAYER_LOCALS_BARRIER_WEAK ;

    sz = env ()->push_locals (d, lt);
    twait { _pub_iface->publish (this, nm, mkevent (status, dummy)); }
    env ()->pop_to (sz);
    ev->trigger (status);
  }

  //--------------------------------------------------------------------

  tamed void
  eval_t::publish_file (ptr<const file_t> file, status_ev_t ev)
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
    } else if (!file->data ()) {
      // skip an empty file
    } else {
      md = file->metadata ();
      push_metadata (md);
      sz = env ()->push_locals (md->to_binding ());
      twait { file->data ()->publish (this, mkevent (status)); }
      env ()->pop_to (sz);
      pop_metadata ();

      // If there was an exit() call inside the file, reset that
      // flag here.
      control ()->reset_file ();
    }
    ev->trigger (status);
  }

  //--------------------------------------------------------------------

  str
  eval_t::set_cwd (str s)
  {
    str ret = _cwd;
    _cwd = s;
    return ret;
  }

  //--------------------------------------------------------------------

  void eval_t::output (zstr z) { out ()->output (z); }
  void eval_t::output (str z) { out ()->output (z); }
  void eval_t::output (zstr orig, zstr wss) { out ()->output (orig, wss); }

  //--------------------------------------------------------------------

  void
  eval_t::output_errs (const xpub3_errstrs_t &errs, err_type_t typ)
  {
    for (size_t i = 0; i < errs.size (); i++) {
      output_err (errs[i], typ);
    }
  }

  //--------------------------------------------------------------------

  ptr<bindtab_t>
  eval_t::push_bindings (env_t::layer_type_t lt)
  {
    return env ()->push_bindings (lt, is_config ());
  }

  //--------------------------------------------------------------------
  
  bool eval_t::is_config () const { return _opts & P_CONFIG; }

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

  //--------------------------------------------------------------------

  bool
  runloc_t::set_active (bool b)
  {
    bool ret = _active;
    _active = b;
    return ret;
  }

  //--------------------------------------------------------------------

  void
  runloc_t::profile_report (profiler_buf_t *b) const
  {
    str fn = filename ();
    int i = lineno ();
    b->add_ch (' ');
    b->add_s (fn);
    b->add_ch (':');
    b->add_i (i);
  }

  //====================================== loc_stack_t ==================

  loc_stack_t::loc_stack_t ()
  {
    profiler_t::singleton()->register_stack (this);
  }

  //-----------------------------------------------------------------------

  // Make sure that we account for copies independently of the
  // original sources. Especially useful in cloning 
  loc_stack_t::loc_stack_t (const loc_stack_t &l)
    : vec<runloc_t> (l)
  {
    profiler_t::singleton()->register_stack (this);
  }

  //-----------------------------------------------------------------------

  loc_stack_t::~loc_stack_t ()
  {
    profiler_t::singleton()->unregister_stack (this);
  }

  //-----------------------------------------------------------------------

  void
  loc_stack_t::profile_report (profiler_buf_t *b, int64_t i) const
  {
    if (!size ()) { return; }

    b->add_i (i);
    b->add_ch (':');

    // print the top two stack frames, if available
    for (int i=0; i<2; i++) {
        int pos = int(size()) - i - 1;
        if (pos < 0) {
            b->add_cc(" empty");
        } else {
            (*this)[pos].profile_report(b);
        }
    }
    if (!back().active()) {
        b->add_cc (" (inactive)");
    }
    b->flush ();
  }

  //-----------------------------------------------------------------------

  bool
  loc_stack_t::set_active (bool b)
  {
    bool ret = false;
    if (size ()) { ret = back().set_active (b); }
    return ret;
  }

  //-----------------------------------------------------------------------

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

