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

  //----------------------------------------------------------------------

  void
  location_t::toplevel ()
  {
    _filename = "<top-level>";
    _lineno = 0;
  }

  //----------------------------------------------------------------------

  void
  location_t::set_filename (str n, bool preserve_lineno)
  {
    _filename = n;
    if (!preserve_lineno) _lineno = 1;
  }

  // ======================================================================

};
