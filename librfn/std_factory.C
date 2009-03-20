
#include "okrfn.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  std_factory_t::std_factory_t ()
  {
    _tab.insert ("random", &random_t::constructor);
  }

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  std_factory_t::alloc (const str &s, ptr<expr_list_t> l, int lineno, str *err)
  {
    ptr<runtime_fn_t> ret;
    constructor_t *c = _tab[s];
    if (c) {
      ret = (**c)(s, l, lineno, err);
    } else if (err) {
      strbuf b ("no such function in std library: '%s'", s.cstr ());
      *err = b;
    }
    return ret;
  }


  //-----------------------------------------------------------------------

}
