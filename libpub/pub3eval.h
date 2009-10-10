// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3expr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  //-----------------------------------------------------------------------

  class env_t {
  public:

    enum { FRAME_UNIVERSALS = 0, FRAME_GLOBALS = 1 };

    env_t (ptr<bindtab_t> u) : _universals (u) {}
    size_t push_locals (ptr<bindtab_t> t);
    size_t push_universal_refs (ptr<bindtab_t> t);
    void pop_to (size_t s);
    ptr<const expr_t> lookup_val (const str &nm) const;
    ptr<mref_t> lookup_ref (const str &nm) const;

    typedef enum { LAYER_UNIVERSALS = 0,
		   LAYER_GLOBALS = 1,
		   LAYER_LOCALS = 2,
		   LAYER_UNIREFS = 3 } layer_type_t;
    
    struct stack_layer_t {
      stack_layer_t (ptr<bindtab_t> b, layer_type_t t) 
	: _bindings (b), _typ (t) {}
      ptr<bindtab_t> _bindings;
      layer_type_t _typ;
    };

  protected:
    ptr<bindtab_t> _universals;
    ptr<bindtab_t> _globals;
    vec<stack_layer_t> _stack;
  };

  //-----------------------------------------------------------------------

  typedef event<xpub_status_t>::ref status_ev_t;

  //-----------------------------------------------------------------------

  // Not sure what's going to be here yet...
  class output_t {


  };

  //-----------------------------------------------------------------------

  class eval_t {
  public:

    enum { EVAL_INIT = -2, EVAL_DONE = -1 };

    eval_t (ptr<env_t> e, ptr<output_t> o); 
    ~eval_t ();

    ptr<output_t> output () const { return _output; }
    ptr<env_t> env () { return _env; }
    ptr<const env_t> env () const { return _env; }

    bool set_loud (bool b);
    bool loud () const { return _loud && !_silent; }
    bool set_silent (bool b);

    void set_in_json () { _in_json = true; }
    bool in_json () const { return _in_json; }
    ptr<const expr_t> lookup_val (const str &nm) const;

  private:

    ptr<env_t> _env;
    ptr<output_t> _output;
    bool _loud;
    bool _silent;
    ssize_t _stack_p;
    bool _in_json;
  };

  //-----------------------------------------------------------------------

  class publish_t : public eval_t {

  };

  //-----------------------------------------------------------------------
};
