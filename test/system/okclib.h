// -*-c++-*-

#pragma once

#include "pub.h"
#include "pub3lib.h"
#include "okrfn.h"

// OkCupid's Runtime library...

namespace okclib {

  //-----------------------------------------------------------------------

  using namespace pub3;
  extern const char *libname;

  PUB3_COMPILED_FN(money, "i|s");
  PUB3_COMPILED_FN(commafy, "i");
  PUB3_COMPILED_FN(bit_set, "u|u");

  //-----------------------------------------------------------------------
  
  class lib_t : public pub3::library_t {
  public:
    lib_t ();
    static ptr<lib_t> alloc ();
  };

  //-----------------------------------------------------------------------

};
