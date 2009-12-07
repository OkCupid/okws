
#include "okrfnlib.h"
#include "okformat.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  random_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    u_int64_t def_range = 10;
    u_int64_t l = 0;
    u_int64_t h = def_range;

    if (args.size () > 0) { l = args[0]._u; }
    if (args.size () > 1) { l = args[1]._u; }

    int64_t d = h - l;

    if (d <= 0) {
      strbuf b ("range for random must be greater than 0 (got %" PRId64 ")", d);
      report_error (e, b);
      h = l + def_range;
    }

    int64_t range = h - l;
    assert (range > 0);

    u_int64_t v = (random () % range) + l;
    return expr_uint_t::alloc (v);
  }

  //-----------------------------------------------------------------------


};
