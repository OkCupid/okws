// -*-c++-*-
/* $Id$ */

#pragma once

#include "pub3expr.h"
#include "pub3obj.h"
#include "pub3pub.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class output_t {
  public:
    output_t (opts_t o = 0) : _opts (o), _muzzle (false) {}
    virtual ~output_t () {}

    virtual void output_err (runloc_t loc, str msg, err_type_t t) = 0;
    virtual void output_err (const loc_stack_t &stack, str msg, 
			     err_type_t t) = 0;

    virtual void output (zstr s) = 0;
    virtual void output (str s) = 0;
    static ptr<output_t> alloc (zbuf *z);
    bool push_muzzle (bool b);
    void pop_muzzle (bool b);

    void pub3_add_error (runloc_t loc, str msg, err_type_t typ);
  protected:
    opts_t _opts;
    pub3::obj_list_t _err_obj;
    bool _muzzle;
  };

  //-----------------------------------------------------------------------

  class output_std_t : public output_t {
  public:
    output_std_t (zbuf *z, opts_t o = 0) : output_t (o), _out (z) {}
    void output_err (runloc_t loc, str msg, err_type_t t);
    void output_err (const loc_stack_t &stack, str msg, err_type_t t);
    void output (zstr s);
    void output (str s);

  private:
    zbuf *_out;
  };

  //-----------------------------------------------------------------------

  class output_silent_t : public output_t {
  public:
    output_silent_t (opts_t o = 0) : output_t (o) {}

    void output_err (runloc_t loc, str msg, err_type_t t);
    void output_err (const loc_stack_t &stack, str msg, err_type_t t);
    void output (zstr s);
    void output (str s);

    static ptr<output_silent_t> alloc ();
  };

  //--------------------------------------------------------------------
};
