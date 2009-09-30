// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

namespace pub3 {

  //-----------------------------------------------------------------------
  
  typedef int lineno_t;

  //-----------------------------------------------------------------------

  struct location_t {
    location_t () : _lineno (0) {}
    location_t (str f, lineno_t l) : _filename (f), _lineno (l) {}
    str to_str () const;
    str _filename;
    lineno_t _lineno;
  };
  
  //-----------------------------------------------------------------------

};


