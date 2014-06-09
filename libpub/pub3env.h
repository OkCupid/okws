// -*-c++-*-
/* $Id$ */

#pragma once

#include "pub3expr.h"
#include "pub3debug.h"

namespace pub3 {

  //======================================================================
  //
  // Global data shared across all pub objects -- such as global bindings
  // published from config files are startup.
  //
  class singleton_t {
  public:
    singleton_t ();
    static ptr<singleton_t> get ();
    ptr<expr_dict_t> universals () { return _universals; }

    const vec<ptr<bindtab_t> > *libraries () const { return &_libraries; }
    vec<ptr<bindtab_t> > *libraries () { return &_libraries; }
    void import (ptr<bindtab_t> l) { _libraries.push_back (l); }

  private:
    ptr<expr_dict_t> _universals;
    vec<ptr<bindtab_t> > _libraries;
  };

  //
  //======================================================================

  //-----------------------------------------------------------------------

  class env_t : public virtual dumpable_t {
  public:

    env_t (ptr<bindtab_t> u, ptr<bindtab_t> g = NULL);
    void pop_to (size_t s);
    ptr<const expr_t> lookup_val (const str &nm) const;
    size_t stack_size () const;
    ptr<mref_t> lookup_ref (const str &nm) const;
    void add_global_binding (const str &nm, ptr<expr_t> v);

    ptr<bindtab_t> library () { return _library; }
    ptr<bindtab_t> globals () { return _globals; }
    ptr<bindtab_t> universals () { return _universals; }

    size_t bind_globals (ptr<bindtab_t> t);
    ptr<env_t> clone () const;

    //
    // LAYER_LOCALS_BARRIER -- stops resolution from descending past
    //   the barrier, whether during and evaluation or a closure capture.
    //   Is used by default to push on a new file or function call.
    //
    // LAYER_LOCALS_BARRIER_WEAK -- doesn't stop resolution from 
    //   descending past, but does stop a closure capture.
    //   Is used when P_STRICT_INCLUDE_SCOPING is turned off to 
    //   mark include file boundaries in the stack
    //
    typedef enum { LAYER_NONE = -1,
		   LAYER_UNIVERSALS = 0,
		   LAYER_GLOBALS = 1,
		   LAYER_LOCALS = 2,
		   LAYER_LOCALS_BARRIER = 3,
		   LAYER_LOCALS_BARRIER_WEAK = 4,
		   LAYER_UNIREFS = 5, 
		   LAYER_LIBRARY = 6 } layer_type_t;

    size_t push_locals (ptr<bind_interface_t> t, 
			layer_type_t typ = LAYER_LOCALS);

    static str layer_type_to_str (layer_type_t lt);

    // Used primarily by bind() and unbind() library functions
    ptr<bindtab_t> lookup_layer (const str &nm, layer_type_t lt, 
				 bool creat) const;

    ptr<bindtab_t> push_bindings (layer_type_t typ, bool is_cfg);
    void push_references (ptr<const bindlist_t> l, layer_type_t lt);

    struct stack_layer_t {
      stack_layer_t (ptr<bind_interface_t> b, layer_type_t t) 
	: _bindings (b), _typ (t) {}
      stack_layer_t () : _typ (LAYER_NONE) {}
      ptr<bind_interface_t> _bindings;
      layer_type_t _typ;
      bool is_local () const;
      bool is_barrier () const { return _typ == LAYER_LOCALS_BARRIER; } 
      ptr<expr_list_t> to_list () const;
    };

    const char *get_obj_name () const { return "env_t"; }
    lineno_t dump_get_lineno () const { return 0; }
    void v_dump (dumper_t *d) const;
    typedef vec<stack_layer_t> stack_t;

    size_t push_lambda (ptr<bind_interface_t>, const stack_t *stk);
    size_t push_barrier ();
    void capture_closure (stack_t *out) const;
    ptr<expr_list_t> to_list () const;
    
  protected:
    size_t dec_stack_pointer (stack_layer_t l, size_t i) const;
    ssize_t descend_to_barrier () const;

    ptr<bindtab_t> _library;
    ptr<bindtab_t> _universals;
    ptr<bindtab_t> _globals;
    stack_t _stack;
    size_t _global_frames;

  public:
    const stack_t &stack() const { return _stack; }
  };

  //=======================================================================
};
