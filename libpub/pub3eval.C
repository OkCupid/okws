
#include "pub3eval.h"
#include "pub3out.h"
#include "pub3hilev.h"

namespace pub3 {

  //===================================== singleton_t =====================

  singleton_t::singleton_t () :
    _universals (New refcounted<expr_dict_t> ()) {}
  
  //-----------------------------------------------------------------------

  ptr<singleton_t> 
  singleton_t::get ()
  {
    static ptr<singleton_t> g;
    if (!g) { g = New refcounted<singleton_t> (); }
    return g;
  }

  //======================================== env_t ========================

  env_t::env_t  (ptr<bindtab_t> u, ptr<bindtab_t> g) 
    : _universals (u), 
      _globals (g ? g : New refcounted<bindtab_t> ())
  {
    const vec<ptr<bindtab_t> > *lib = singleton_t::get ()->libraries ();
    for (size_t i = 0; i < lib->size (); i++) {
      _stack.push_back (stack_layer_t ((*lib)[i], LAYER_LIBRARY));
    }
    _stack.push_back (stack_layer_t (_universals, LAYER_UNIVERSALS));
    _stack.push_back (stack_layer_t (_globals, LAYER_GLOBALS));
    _global_frames = _stack.size () - 1;
  }

  //-----------------------------------------------------------------------

  void
  env_t::add_global_binding (const str &n, ptr<expr_t> x)
  {
    _globals->insert (n, x);
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::push_locals (ptr<bind_interface_t> t, bool barrier)
  {
    size_t ret = _stack.size ();
    if (t) {
      layer_type_t lt = barrier ? LAYER_LOCALS_BARRIER : LAYER_LOCALS;
      _stack.push_back (stack_layer_t (t, lt));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::push_barrier ()
  {
    size_t ret = _stack.size ();
    _stack.push_back (stack_layer_t (NULL, LAYER_LOCALS_BARRIER));
    return ret;
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::push_lambda (ptr<bind_interface_t> bi, const env_t::stack_t *cls_stk)
  {
    size_t ret = _stack.size ();
    if (cls_stk) {
      if (cls_stk->size () == 0 || 
	  (*cls_stk)[0]._typ != LAYER_LOCALS_BARRIER)  {
	push_barrier ();
      }
      _stack += *cls_stk; 
    }

    push_locals (bi, false);
    return ret;
  }

  //-----------------------------------------------------------------------
  
  ptr<bindtab_t>
  env_t::push_bindings (layer_type_t typ)
  {
    ptr<bindtab_t> ret;
    switch (typ) {
    case LAYER_LIBRARY: ret = _library; break;
    case LAYER_UNIVERSALS: ret = _universals; break;
    case LAYER_GLOBALS: ret = _globals; break;
    case LAYER_LOCALS:
      ret = New refcounted<bindtab_t> ();
      _stack.push_back (stack_layer_t (ret, typ));
      break;
    default:
      fatal << "unexpected stack layer type!\n";
      break;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  env_t::push_references (ptr<const bindlist_t> l, layer_type_t typ)
  {
    if (typ == LAYER_UNIVERSALS) {
      _stack.push_back (stack_layer_t (l->keys_only (), LAYER_UNIREFS));
    }
  }
  
  //-----------------------------------------------------------------------

  void env_t::pop_to (size_t i) { _stack.setsize (i); }
  size_t env_t::stack_size () const { return _stack.size (); }

  //-----------------------------------------------------------------------

  size_t
  env_t::dec_stack_pointer (stack_layer_t l, size_t i) const
  {
    size_t ret;
    if (l.is_barrier ()) { ret = _global_frames; }
    else                 { ret = i - 1; }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<mref_t>
  env_t::lookup_ref (const str &nm) const
  {
    ptr<bindtab_t> found;
    ssize_t i = _stack.size () - 1;

    while (!found && i >= 0) {
      stack_layer_t l = _stack[i];
      if (l._bindings && l._bindings->lookup (nm)) {
	if (l._typ == LAYER_UNIREFS) { found = _universals; }
	else { found = l._bindings->mutate (); }
      }

      // When we hit a scope barrier -- like a file inclusion or
      // a lambda call -- then jump over stack frames until we get
      // to the base layers (with the globals and universals).
      i = dec_stack_pointer (l, i);
    }

    if (!found) { found = _globals; }
    return New refcounted<mref_dict_t> (found, nm);
  }

  //-----------------------------------------------------------------------

  void
  env_t::v_dump (dumper_t *d) const
  {
    for (ssize_t i = _stack.size () - 1; i >=0 ; i--) {
      d->dump (strbuf ("layer(%d) ", int (_stack[i]._typ)), false);
      ptr<bind_interface_t> bi;
      if ((bi = _stack[i]._bindings)) { 
	ptr<bindtab_t::const_iterator_t> it = bi->iter ();
	const str *key;
	ptr<expr_t> value;
	d->begin_obj ("bindtab", NULL, 0);
	while ((key = it->next (&value))) {
	  s_dump (d, *key, value);
	}
	d->end_obj ();
      } else {
	d->dump ("{}", true);
      }
    }
  }

  //-----------------------------------------------------------------------
  
  void
  env_t::capture_closure (stack_t *out) const
  {
    size_t b = descend_to_barrier ();
    for (size_t i = b; i < _stack.size (); i++) {
      out->push_back (_stack[i]);
    }
  }

  //-----------------------------------------------------------------------
  
  ssize_t
  env_t::descend_to_barrier () const
  {
    ssize_t bottom_frame = _stack.size () - 1;
    bool go = true;
    while (go && bottom_frame >= 0) {
      switch (_stack[bottom_frame]._typ) {
      case LAYER_UNIREFS:
      case LAYER_LOCALS:
	bottom_frame--;
	break;
      case LAYER_NONE:
      case LAYER_UNIVERSALS:
      case LAYER_GLOBALS:
      case LAYER_LIBRARY:
	bottom_frame++;
	go = false;
	break;
      case LAYER_LOCALS_BARRIER:
	go = false;
	break;
      }
    }
    return bottom_frame;
  }

  //================================================ eval_t ================

  eval_t::eval_t (ptr<env_t> e, ptr<output_t> o)
    : _env (e), _output (o), _loud (false), _silent (false) {}

  //-----------------------------------------------------------------------

  eval_t::~eval_t () {}

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

  ptr<const expr_t>
  env_t::lookup_val (const str &nm) const
  {
    ptr<const expr_t> x;
    ssize_t i = _stack.size () - 1;
    while (!x && i >= 0) {
      stack_layer_t l = _stack[i];
      if (l._bindings && l._bindings->lookup (nm, &x)) {
	if (l._typ == LAYER_UNIREFS) { _universals->lookup (nm, &x); }
	if (!x) { x = expr_null_t::alloc (); }
      }
      i = dec_stack_pointer (l, i);
    }
    return x;
  }

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
    set_lineno (ln);
    output_err (msg, P_ERR_EVAL);
  }

  //-----------------------------------------------------------------------

  void eval_t::report_error (str msg, location_t l) 
  { report_error (msg, l._lineno); }

  //-----------------------------------------------------------------------

};
