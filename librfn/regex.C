#include "okrfn.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  match_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> x = args[0]._r;
    str s = args[1]._s;
    bool b = x->match (s);
    return expr_bool_t::alloc (b);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  search_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<rxx> x = args[0]._r;
    str s = args[1]._s;
    bool b = x->search (s);
    return expr_bool_t::alloc (b);
  }

  //-----------------------------------------------------------------------

};
