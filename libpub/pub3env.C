
#include "pub3eval.h"
#include "pub3out.h"
#include "pub3hilev.h"

namespace pub3 {

  //===================================== singleton_t =====================

  singleton_t::singleton_t () :
    _universals (expr_dict_t::alloc ()) {}
  
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
      _globals (g ? g : bindtab_t::alloc ())
  {
    const vec<ptr<bindtab_t> > *lib = singleton_t::get ()->libraries ();
    for (size_t i = 0; i < lib->size (); i++) {
      _stack.push_back (stack_layer_t (cow_bindtab_t::alloc ((*lib)[i]), 
				       LAYER_LIBRARY));
    }
    _stack.push_back (stack_layer_t (_universals, LAYER_UNIVERSALS));
    _stack.push_back (stack_layer_t (_globals, LAYER_GLOBALS));
    _global_frames = _stack.size () - 1;
  }

  //-----------------------------------------------------------------------

  ptr<expr_list_t>
  env_t::stack_layer_t::to_list () const
  {
    ptr<expr_list_t> ret = expr_list_t::alloc ();
    ret->push_back (expr_str_t::alloc (layer_type_to_str (_typ)));
    ptr<expr_t> b;
    if (_bindings) { b = _bindings->copy_to_dict (); }
    else { b = expr_null_t::alloc (); }
    ret->push_back (b);
    return ret;
  }

  //-----------------------------------------------------------------------

  bool
  env_t::stack_layer_t::is_local () const
  {
    return _typ == LAYER_LOCALS || _typ == LAYER_LOCALS_BARRIER ||
      _typ == LAYER_LOCALS_BARRIER_WEAK;
  }

  //-----------------------------------------------------------------------

  str
  env_t::layer_type_to_str (layer_type_t lt)
  {
    str ret;
    switch (lt) {
    case LAYER_NONE: ret = "none"; break;
    case LAYER_UNIVERSALS: ret = "universals"; break;
    case LAYER_GLOBALS: ret = "globals"; break;
    case LAYER_LOCALS: ret = "locals"; break;
    case LAYER_LOCALS_BARRIER: ret = "locals_barrier"; break;
    case LAYER_LOCALS_BARRIER_WEAK: ret = "locals_barrier_weak"; break;
    case LAYER_UNIREFS: ret = "unirefs"; break;
    case LAYER_LIBRARY: ret = "library"; break;
    default: ret = "none"; break;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<expr_list_t>
  env_t::to_list () const
  {
    ptr<expr_list_t> l = expr_list_t::alloc ();
    for (size_t i = 0; i < _stack.size (); i++) {
      ptr<expr_list_t> layer = _stack[i].to_list ();
      l->push_back (layer);
    }
    return l;
  }

  //-----------------------------------------------------------------------

  void
  env_t::add_global_binding (const str &n, ptr<expr_t> x)
  {
    _globals->insert (n, x);
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::push_locals (ptr<bind_interface_t> t, layer_type_t lt)
  {
    size_t ret = _stack.size ();
    if (t) {
      _stack.push_back (stack_layer_t (t, lt));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::bind_globals (ptr<bindtab_t> t)
  {
    size_t ret = _stack.size ();
    if (!t) { 
      /* noop */ 
    } else if (!_globals || !_globals->size ()) {
      // This is a skeezy optimization -- if there's nothing in the
      // globals array, then it's ok to just replace it.  but make
      // sure we replace it in both places!
      _globals = t; 
      _stack[_global_frames]._bindings = t;
    } else {
      (*_globals) += *t; 
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

    push_locals (bi, LAYER_LOCALS);
    return ret;
  }

  //-----------------------------------------------------------------------
  
  ptr<bindtab_t>
  env_t::push_bindings (layer_type_t typ, bool is_cfg)
  {
    ptr<bindtab_t> ret;
    switch (typ) {
    case LAYER_LIBRARY: ret = _library; break;
    case LAYER_UNIVERSALS: ret = is_cfg ? _globals : _universals; break;
    case LAYER_GLOBALS: ret = _globals; break;
    case LAYER_LOCALS:
      ret = bindtab_t::alloc ();
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

  //
  // Lookup the first feasible bindtab that refers to 'nm', where
  // the bindtab is of the specified scope (local, global, universals...)
  //
  // For unis, and globals, this is pretty straight forward.  For
  // locals, find an applicable binding, and if none found, create one
  // on the topmost local layer.
  //
  // Used primarily by the runtime function 'bind' and 'unbind'
  //
  ptr<bindtab_t>
  env_t::lookup_layer (const str &nm, env_t::layer_type_t lt, bool creat) const
  {
    ptr<bindtab_t> found;
    ssize_t i = _stack.size () - 1;

    switch (lt) {
    case LAYER_LOCALS:
      {
	bool go = true;
	ptr<bind_interface_t> top;
	while (go && !found && i >= 0 && _stack[i].is_local ()) {
	  stack_layer_t l = _stack[i];
	  if (!top) { top = l._bindings; }
	  if (l._bindings && l._bindings->lookup (nm)) { 
	    found = l._bindings->mutate ();
	  } 
	  else if (l.is_barrier ()) { go = false; }
	  else { i--; }
	}
	if (!found && top && creat) { found = top->mutate (); }
      }
      break;
    case LAYER_GLOBALS:
      found = _globals;
      break;
    case LAYER_UNIVERSALS:
      found = _universals;
      break;
    default:
      break;
    }
    return found;
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
      str t = layer_type_to_str (_stack[i]._typ);
      d->dump (strbuf ("layer(%s) ", t.cstr ()), false);
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
      case LAYER_LOCALS_BARRIER_WEAK:
	go = false;
	break;
      }
    }
    return bottom_frame;
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
  
  ptr<env_t> env_t::clone () const { return New refcounted<env_t> (*this); }

  //-----------------------------------------------------------------------
};
