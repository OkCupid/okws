
#include "okrfn.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  random_t::random_t (const str &n, ptr<expr_list_t> al, int ln, 
		      ptr<expr_t> l, ptr<expr_t> h)
    : runtime_fn_t (n, al, ln), _low (l), _high (h) {}

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t>
  random_t::constructor (const str &n, ptr<expr_list_t> e, int lineno, str *err)
  {
    ptr<runtime_fn_t> ret;
    ptr<expr_t> l, h;
    size_t narg = e ? e->size () : size_t (0);
    bool ok = true;

    if (narg == 0 || narg > 2) {
      ok = false;
      *err = "random() takes 1 or 2 arguments";
    } else if (narg == 1) {
      h = (*e)[0];
    } else {
      l = (*e)[0];
      h = (*e)[1];
    }

    if (ok) {
      ret = New refcounted<random_t> (n, e, lineno, l, h);
    }
    return ret;
  }

  //-----------------------------------------------------------------------


};
