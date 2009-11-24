// -*-c++-*-
/* $Id$ */

#pragma once

#include "pub3expr.h"
#include "pub3debug.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class env_t : public virtual dumpable_t {
  public:

    env_t (ptr<bindtab_t> u, ptr<bindtab_t> g = NULL);
    size_t push_locals (ptr<bind_interface_t> t);
    size_t push_universal_refs (ptr<bind_interface_t> t);
    void pop_to (size_t s);
    ptr<const expr_t> lookup_val (const str &nm) const;
    size_t stack_size () const;
    ptr<mref_t> lookup_ref (const str &nm) const;
    void add_global_binding (const str &nm, ptr<expr_t> v);

    // Replace the universals with this bindtab;  no way to undo
    // this yet since it's not needed, but eventually might be nice.
    void replace_universals (ptr<bindtab_t> u);

    typedef enum { LAYER_NONE = -1,
		   LAYER_UNIVERSALS = 0,
		   LAYER_GLOBALS = 1,
		   LAYER_LOCALS = 2,
		   LAYER_UNIREFS = 3 } layer_type_t;

    size_t push_bindings (ptr<bind_interface_t> t, layer_type_t lt);
    ptr<bindtab_t> push_bindings (layer_type_t typ);
    void push_references (ptr<const bindlist_t> l, layer_type_t lt);

    struct stack_layer_t {
      stack_layer_t (ptr<bind_interface_t> b, layer_type_t t) 
	: _bindings (b), _typ (t) {}
      stack_layer_t () : _typ (LAYER_NONE) {}
      ptr<bind_interface_t> _bindings;
      layer_type_t _typ;
    };

    void overwrite_universals (ptr<const bind_interface_t> t);
    const char *get_obj_name () const { return "env_t"; }
    lineno_t dump_get_lineno () const { return 0; }
    void v_dump (dumper_t *d) const;

  protected:

    ptr<bindtab_t> _universals;
    ptr<bindtab_t> _globals;
    vec<stack_layer_t> _stack;
  };

  //-----------------------------------------------------------------------

  // Forward-declared; more information available in pub3out.h
  class output_t;
  
  // Forward-declared class available in pub3file.h
  class metadata_t;

  //-----------------------------------------------------------------------

  class eval_t {
  public:

    enum { EVAL_INIT = -2, EVAL_DONE = -1 };

    eval_t (ptr<env_t> e, ptr<output_t> o); 
    ~eval_t ();

    ptr<output_t> out () const { return _output; }
    ptr<env_t> env () { return _env; }
    ptr<const env_t> env () const { return _env; }

    bool set_loud (bool b);
    bool loud () const { return _loud && !_silent; }
    bool set_silent (bool b);

    ptr<const expr_t> lookup_val (const str &nm) const;
    ptr<mref_t> lookup_ref (const str &nm) const;

    location_t location (lineno_t l) const;
    bool push_muzzle (bool b);
    void pop_muzzle (bool b);

    void report_error (str msg, location_t l);
    void report_error (str msg, lineno_t l);

    virtual void set_lineno (lineno_t l) {}
    virtual void output_err (str msg, err_type_t typ) {}
    virtual ptr<const metadata_t> current_metadata () const { return NULL; }

  protected:

    ptr<env_t> _env;
    ptr<output_t> _output;
    bool _loud;
    bool _silent;
  };

  //-----------------------------------------------------------------------
};
