
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

  
  // This is an example of a pub3 filter that can be used like this:
  //      Your balance is: %{-1234567|money("$")}
  // which will produce the following as output:
  //      Your balance is: -$12,345.67
  //
  ptr<const expr_t>
  money_t::v_eval_2 (eval_t *pub, const vec<arg_t> &args) const
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

  ptr<const expr_t>
  commafy_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    int64_t n = args[0]._i;
    const char *sign = "";
    
    if (n < 0) {
      sign = "-";
      n = 0 - n;
    }
 
    str ns = comma_delimit (n);
    strbuf b ("%s%s", sign, ns.cstr ());
    return expr_str_t::alloc (b);
  }

  //-------------------------------------------------------------------
  
  ptr<const expr_t>
  bit_set_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    u_int64_t num = args[0]._u;
    size_t len = args.size () > 1 ? size_t (args[1]._i) : 8 * sizeof (num);
    ptr<expr_list_t> out = expr_list_t::alloc ();
    for (size_t i = 0; i < len; i++) {
      if (num & (1ULL << i)) {
	ptr<expr_t> x = New refcounted<expr_int_t>(i);
	out->push_back(x);
      }
    }
    return out;
  }
  
  //-----------------------------------------------------------------------
  
  const char *libname = "okclib";

  //-----------------------------------------------------------------------

  lib_t::lib_t () : library_t () 
  {
#define F(f) \
    _functions.push_back (New refcounted<f##_t> ())
    F(money);
    F(commafy);
    F(bit_set);
#undef F

  }

  //-----------------------------------------------------------------------

  ptr<lib_t> lib_t::alloc () { return New refcounted<lib_t> (); }

  //-----------------------------------------------------------------------

};

