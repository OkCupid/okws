
#include "okrfn.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  std_factory_t::std_factory_t ()
  {
    _tab.insert ("rand",   &random_t::constructor);
    _tab.insert ("len",    &len_t::constructor);
    _tab.insert ("range",  &range_t::constructor);
    _tab.insert ("isnull", &is_null_t::constructor);
    _tab.insert ("match",  &rxx_fn_t::constructor);
    _tab.insert ("search", &rxx_fn_t::constructor);

    _tab.insert ("tolower", &scalar_fn_t::constructor<tolower_t, 1>);
    _tab.insert ("toupper", &scalar_fn_t::constructor<toupper_t, 1>);
    _tab.insert ("html_escape", &scalar_fn_t::constructor<html_escape_t, 1>);
    _tab.insert ("json_escape", &scalar_fn_t::constructor<json_escape_t, 1>);
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
