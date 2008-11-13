
#include "okwsconf.h"
#ifdef HAVE_SSL

# include "oksslutil.h"
# include <openssl/rand.h>

namespace okssl {

  //-----------------------------------------------------------------------
  
  bool ssl_ok (int rc) { return rc == 1; }
  
  //-----------------------------------------------------------------------
  
  static BIO *_bio_err;
  
  //-----------------------------------------------------------------------

  static bool
  init_rand ()
  {
#define BUFSZ 40
    unsigned char foo[BUFSZ];
    RAND_get_rand_method() ->bytes (foo, BUFSZ);
#undef BUFSZ
    return true;
  }

  //-----------------------------------------------------------------------
  
  bool
  init_ssl_internals ()
  {
    if (!_bio_err) {
      SSL_library_init ();
      SSL_load_error_strings ();
      _bio_err = BIO_new_fp (stderr, BIO_NOCLOSE);
    }
    if (!init_rand ())
      warn ("Failed to initialize random number generator in SSL\n");
  
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

#endif /* HAVE_SSL */

