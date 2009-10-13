// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "async.h"

namespace pub3 {

  //-----------------------------------------------------------------------
  
  typedef int lineno_t;

  //-----------------------------------------------------------------------

  struct location_t {
    location_t () : _lineno (0) {}
    location_t (str f, lineno_t l) : _filename (f), _lineno (l) {}
    location_t (lineno_t l) : _lineno (l) {}
    void set_filename (str f) { _filename = f; _lineno = 1; }
    str to_str () const;
    str _filename;
    lineno_t _lineno;
  };
  
  //-----------------------------------------------------------------------

  class tri_bool_t {
  public:
    tri_bool_t () : _v (-1) {}
    tri_bool_t (bool b) : _v (b) {}
    bool is_set () const { return _v >= 0; }
    bool value () const { return _v > 0; }
    void set (bool b) { _v = b; }
  private:
    int _v;
  };

  //-----------------------------------------------------------------------

};


