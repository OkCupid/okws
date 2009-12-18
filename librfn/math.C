
#include "okrfn.h"
#include "okformat.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  random_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    u_int64_t def_range = 10;
    u_int64_t l = 0;
    u_int64_t h = def_range;

    if (args.size () == 2) {
      l = args[0]._u;
      h = args[1]._u;
    } else if (args.size () == 1) {
      h = args[0]._u;
    }

    int64_t d = h - l;

    if (d <= 0) {
      strbuf b ("range for random must be greater than 0 (got %" PRId64 ")", d);
      report_error (p, b);
      h = l + def_range;
    }

    int64_t range = h - l;
    assert (range > 0);

    u_int64_t v = (random () % range) + l;
    return expr_uint_t::alloc (v);
  }

  //-----------------------------------------------------------------------


};
