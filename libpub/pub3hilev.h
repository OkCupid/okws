// -*-c++-*-

#pragma once

#include "pub3pub.h"
#include "pub3file.h"
#include "pub3ast.h"
#include "pub3expr.h"

namespace pub3 {

  typedef xpub_status_t status_t;
  typedef event<status_t>::ref status_ev_t;
  typedef event<status_t, ptr<file_t> >::ref getfile_ev_t;
  typedef u_int opts_t;

  //-----------------------------------------------------------------------

  class ok_iface_t {
  public:
    virtual ~ok_iface_t () {}

    virtual void run_full (zbuf *b, str fn, getfile_ev_t ev, ptr<expr_dict_t> d,
			   opts_t opts = 0) = 0;

    virtual void run (zbuf *b, str fn, evb_t ev, ptr<expr_dict_t> d, 
		      opts_t opts = 0) = 0;

    virtual void run_cfg_full (str fn, getfile_ev_t ev, 
			       ptr<expr_dict_t> d = NULL) = 0;

    virtual void run_cfg (str fn, evb_t ev, ptr<expr_dict_t> d = NULL) = 0;

    virtual opts_t opts () const = 0;
    virtual void set_opts (opts_t i) = 0;
  };

  //--------------------------------------------------------------------

};
