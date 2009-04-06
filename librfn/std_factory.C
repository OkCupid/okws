
#include "okrfn.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  std_factory_t::std_factory_t ()
  {
    _tab.insert ("rand",   &random_t::constructor);
    _tab.insert ("len",    &len_t::constructor);
    _tab.insert ("range",  &range_t::constructor);
    _tab.insert ("isnull", &is_null_t::constructor);
  }

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  std_factory_t::alloc (const str &s, ptr<expr_list_t> l, int lineno)
  {
    ptr<runtime_fn_t> ret;
    constructor_t *c = _tab[s];
    str err;

    if (c) {
      ret = (**c)(s, l, lineno, &err);
    } else {
      strbuf b ("no such function in std library: '%s'", s.cstr ());
      err = b;
    }

    if (!c) {
      ret = New refcounted<error_fn_t> (s, l, lineno, err);
    }

    return ret;
  }


  //-----------------------------------------------------------------------

}
