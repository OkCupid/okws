// -*-c++-*-

#pragma once

#include "pub3expr.h"
#include "pub3eval.h"

namespace pub3 {

  typedef int opts_t;

  //-----------------------------------------------------------------------

  class publish_t : public eval_t {
  public:
    publish_t (ptr<bindtab_t> univerals, zbuf *z = NULL);
    publish_t (ptr<env_t> e, ptr<output_t> o) : eval_t (e, o) {}
    void publish (str nm, ptr<bind_interface_t> d, status_ev_t ev, CLOSURE);
    void publish2 (str nm, status_ev_t ev, CLOSURE);
    void output (str s);
    void set_opts (opts_t o) { _opts = o; }
  private:
    opts_t _opts;
  };

  //--------------------------------------------------------------------

  class output_std_t : public output_t {
  public:
    output_std_t (zbuf *z) : output_t (), _out (z) {}
    void output_err (location_t loc, str msg);
  private:
    zbuf *_out;
  };

  //-----------------------------------------------------------------------

  class output_silent_t : public output_t {
  public:
    output_silent_t () : output_t () {}
    void output_err (location_t loc, str msg);
  };

  //-----------------------------------------------------------------------

};
