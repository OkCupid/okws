// -*-c++-*-

#pragma once

#include "pub3expr.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class publish_t : public eval_t {
  public:
    void publish (str nm, ptr<bind_interface_t> d, status_ev_t ev, CLOSURE);
    void publish2 (str nm, status_ev_t ev, CLOSURE);
    void output (str s);
  };

  //--------------------------------------------------------------------

};
