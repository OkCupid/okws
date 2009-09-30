// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub3expr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class env_t {
  public:

    enum { FRAME_UNIVERSALS = 0, FRAME_GLOBALS = 1 };

    env_t (ptr<bindtab_t> u) : _universals (u) {}
    size_t push_frame (ptr<bindtab_t> t);
    void pop_to_frame (size_t i);
    size_t stack_size () const;

  protected:
    ptr<bindtab_t> _universals;
    ptr<bindtab_t> _globals;
    vec<ptr<bindtab_t> > _locals_stack;
  };

  //-----------------------------------------------------------------------

  class eval_t {
  public:

    enum { EVAL_INIT = -2, EVAL_DONE = -1 };

    eval_t (ptr<env_t> e, output_t *o); 
    ~eval_t ();

    output_t *output () const { return _output; }
    ptr<env_t> env () { return _env; }
    ptr<const env_t> env () const { return _env; }

    bool set_loud (bool b);
    bool loud () const { return _loud && !_silent; }
    bool set_silent (bool b);

    void set_in_json () { _in_json = true; }
    bool in_json () const { return _in_json; }

  private:

    ptr<env_t> _env;
    output_t *_output;
    bool _loud;
    bool _silent;
    ssize_t _stack_p;
    bool _in_json;
  };

  //-----------------------------------------------------------------------

};
