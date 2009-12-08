#include "okrfnlib.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  match_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> x = args[0]._r;
    str s = args[1]._s;
    return x->match (s);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  search_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> x = args[0]._r;
    str s = args[1]._s;
    return x->search (s);
  }

  //-----------------------------------------------------------------------

};
