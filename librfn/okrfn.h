// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#pragma once

#include "pub3.h"
#include "qhash.h"
#include "pub3lib.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  using namespace pub3;
  extern const char *libname;

  //-----------------------------------------------------------------------

  // Most of the functions have been moved into okrfn-int.h, so that
  // we can add new functions without having to recompile and relink lib
  // TODO: figure out why we need to expand one level of macro here...
  PUB3_COMPILED_FN_FULL(warn, "s",PUB3_DOC_MEMBERS);
  PUB3_COMPILED_FN_FULL(replace, "srO|b",PUB3_DOC_MEMBERS);
  
  //-----------------------------------------------------------------------

  class lib_t : public pub3::library_t {
  public:
    lib_t ();
    static ptr<lib_t> alloc ();
  };


  //-----------------------------------------------------------------------
};
