
#include "pub3eval.h"
#include "pub3out.h"

namespace pub3 {

  //=======================================================================

  env_t::env_t  (ptr<bindtab_t> u, ptr<bindtab_t> g) : 
    _universals (u), _globals (g ? g : New refcounted<bindtab_t> ())
  {
    _stack.push_back (stack_layer_t (_universals, LAYER_UNIVERSALS));
    _stack.push_back (stack_layer_t (_globals, LAYER_GLOBALS));
  }

  //-----------------------------------------------------------------------

  void
  env_t::add_global_binding (const str &n, ptr<expr_t> x)
  {
    _globals->insert (n, x);
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::push_locals (ptr<bind_interface_t> t)
  {
    return push_bindings (t, LAYER_LOCALS);
  }

  //-----------------------------------------------------------------------

  size_t 
  env_t::push_universal_refs (ptr<bind_interface_t> t)
  {
    return push_bindings (t, LAYER_UNIVERSALS);
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::push_bindings (ptr<bind_interface_t> bi, layer_type_t typ)
  {
    size_t ret = _stack.size ();
    _stack.push_back (stack_layer_t (bi, typ));

    if (typ == LAYER_UNIVERSALS) {
      overwrite_universals (bi);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  env_t::overwrite_universals (ptr<const bind_interface_t> bi)
  {
    const str *key;
    ptr<expr_t> val;
    ptr<bindtab_t::const_iterator_t> it = bi->iter ();
    
    while ((key = it->next (&val))) {
      // don't insert the actual object, just insert a copy.
      if (val) { _universals->insert (*key, val->copy ()); }
    }
  }

  //-----------------------------------------------------------------------

  void
  env_t::pop_to (size_t i)
  {
    _stack.setsize (i);
  }

  //-----------------------------------------------------------------------

  size_t env_t::stack_size () const { return _stack.size (); }

  //-----------------------------------------------------------------------

  ptr<mref_t>
  env_t::lookup_ref (const str &nm) const
  {
    ptr<bindtab_t> found;
    for (ssize_t i = _stack.size () - 1; !found && i >= 0; i--) {
      stack_layer_t l = _stack[i];
      if (l._bindings && l._bindings->lookup (nm)) {
	if (l._typ == LAYER_UNIREFS) { found = _universals; }
	else { found = l._bindings->mutate (); }
      }
    }
    if (!found) { found = _globals; }
    return New refcounted<mref_dict_t> (found, nm);
  }

  //-----------------------------------------------------------------------

  void
  env_t::replace_universals (ptr<bindtab_t> t)
  {
    push_bindings (t, LAYER_UNIVERSALS);
    _universals = t;
  }

  //=======================================================================

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
    for (ssize_t i = _stack.size () - 1; !x && i >= 0; i--) {
      stack_layer_t l = _stack[i];
      if (l._bindings && l._bindings->lookup (nm, &x)) {
	if (l._typ == LAYER_UNIREFS) { _universals->lookup (nm, &x); }
	if (!x) { x = expr_null_t::alloc (); }
      }
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

};
