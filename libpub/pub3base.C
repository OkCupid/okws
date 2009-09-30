#include "pub3base.h"


namespace pub3 {

  // =================================================== location_t =======

  str
  location_t::to_str () const
  {
    strbuf b (_filename);
    if (_lineno) {
      b << ":" << _lineno;
    }
    return b;
  }

  // ======================================================================

};
