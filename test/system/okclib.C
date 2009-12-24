
#include "okclib.h"

namespace okclib {

  //-----------------------------------------------------------------------

  str comma_delimit (int64_t n)
  {
    enum { BUFSZ = 256 };

    char sn[BUFSZ], res[BUFSZ];
    snprintf(sn, BUFSZ, "%" PRIi64 "", n);
    size_t len = strlen(sn);
    size_t i = 0, j = 0;
    
    
    for (; i < len && j < BUFSZ - 1; ++i, ++j) {
      res[j] = sn[i];
      if (i != len - 1 && // Don't put a comma at the end.
	  ((len - i - 1) % 3) == 0) // This math works because we're 
	// counting forwards.
	{
	  ++j;
	  res[j] = ',';
	}
    }
    res[j] = '\0';
    return str(res);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  money_t::v_eval_2 (publish_t *pub, const vec<arg_t> &args) const
  {
    int64_t cents = args[0]._i;
    str currency = args[0]._s;
    if (!currency) currency = "$";

    const char *sign = "";
    if (cents < 0) {
      cents = 0 - cents;
      sign = "-";
    }

    int64_t dollars = cents / 100;
    cents %= 100;
    str ds = comma_delimit (dollars);
    strbuf s ("%s%s%s.%02" PRIi64, sign, currency.cstr (), ds.cstr (), cents);
    return expr_str_t::alloc (s);
  }

  //-----------------------------------------------------------------------

  const char *libname = "okclib";

  //-----------------------------------------------------------------------

  lib_t::lib_t () : library_t () 
  {
    _functions.push_back (New refcounted<money_t> ());

  }

  //-----------------------------------------------------------------------

  ptr<lib_t> lib_t::alloc () { return New refcounted<lib_t> (); }

  //-----------------------------------------------------------------------

};

