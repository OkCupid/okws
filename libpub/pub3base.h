// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "async.h"
#include "tame.h"
#include "pub3prot.h"

namespace pub3 {
    
  //-----------------------------------------------------------------------

  typedef enum { P_ERR_NONE = 0,
		 P_ERR_ERROR = 1,
		 P_ERR_PARSE = 2,
		 P_ERR_WARNING = 3,
		 P_ERR_EVAL = 4 } err_type_t;

  //-----------------------------------------------------------------------
  
  typedef int lineno_t;
  typedef int opts_t;

  //-----------------------------------------------------------------------

  typedef xpub_status_t status_t;
  typedef event<xpub_status_t>::ref status_ev_t;

  //-----------------------------------------------------------------------

  struct location_t {
    location_t () : _lineno (0) {}
    location_t (str f, lineno_t l) : _filename (f), _lineno (l) {}
    location_t (lineno_t l) : _lineno (l) {}
    void set_filename (str f, bool preserve_lineno = false);
    void toplevel ();
    str to_str () const;
    str _filename;
    lineno_t _lineno;
  };
  
  //-----------------------------------------------------------------------

  struct call_location_t {
    call_location_t (const location_t &l, str cn) : _loc(l), _name (cn) {}
    location_t _loc;
    str _name;
  };

  //-----------------------------------------------------------------------

  class tri_bool_t {
  public:
    tri_bool_t () : _v (-1) {}
    tri_bool_t (bool b) : _v (b) {}
    bool is_set () const { return _v >= 0; }
    bool value () const { return _v > 0; }
    void set (bool b) { _v = b; }
    void reset () { _v = -1; }
  private:
    int _v;
  };

  //-----------------------------------------------------------------------

};


