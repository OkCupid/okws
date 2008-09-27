
#include "oksslutil.h"

namespace okssl {

  //-----------------------------------------------------------------------
  
  bool ssl_ok (int rc) { return rc == 1; }
  
  //-----------------------------------------------------------------------
  
  static BIO *_bio_err;
  
  //-----------------------------------------------------------------------
  
  bool
  init_ssl_internals ()
  {
    if (!_bio_err) {
      SSL_library_init ();
      SSL_load_error_strings ();
      _bio_err = BIO_new_fp (stderr, BIO_NOCLOSE);
    }
    return true;
  }
  
  //-----------------------------------------------------------------------
  
  void 
  ssl_complain (const str &s)
  {
    if (s)
      BIO_printf (_bio_err, "%s", s.cstr ());
    ERR_print_errors (_bio_err);
    fflush (stderr);
  }
  
  //-----------------------------------------------------------------------
};

