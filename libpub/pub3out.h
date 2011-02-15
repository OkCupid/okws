// -*-c++-*-
/* $Id$ */

#pragma once

#include "pub3expr.h"
#include "pub3obj.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class output_t {
  public:
    output_t (opts_t o = 0);
    virtual ~output_t () {}

    virtual void output (zstr s) = 0;
    virtual void output (str s) = 0;
    virtual void output (zstr orig, zstr wss);
    void output_err (runloc_t loc, str msg, err_type_t t);
    void output_err (str msg, err_type_t t);
    void output_err (const loc_stack_t &stack, str msg, err_type_t t);

    static ptr<output_t> alloc (zbuf *z, opts_t o = 0);
    bool push_muzzle (bool b);
    void pop_muzzle (bool b);

    void pub3_add_error (const loc_stack_t &stk, str msg, err_type_t typ);
    ptr<expr_t> err_obj ();
    bool wss_boundary (bool b, str tag);
    bool enable_wss (bool b);
    bool get_wss_enabled () const;
    opts_t get_opts () const;
    void set_opts (opts_t o);
    
  protected:
    bool do_wss () const;
    void output_visible_error (str s);
    opts_t _opts;
    pub3::obj_list_t _err_obj;
    bool _muzzle;

    bool _wss_enabled;
    str _wss_boundary;
  };

  //-----------------------------------------------------------------------

  class output_std_t : public output_t {
  public:
    output_std_t (zbuf *z, opts_t o = 0) : output_t (o), _out (z) {}
    void output (zstr s);
    void output (str s);
  private:
    zbuf *_out;
  };

  //-----------------------------------------------------------------------

  class output_silent_t : public output_t {
  public:
    output_silent_t (opts_t o = 0) : output_t (o) {}
    void output (str s) {}
    void output (zstr s) {}
    static ptr<output_silent_t> alloc (opts_t o = 0);
  };

  //--------------------------------------------------------------------
};
