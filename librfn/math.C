
#include "okrfnlib.h"
#include "okformat.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  scalar_obj_t
  random_t::eval_internal (eval_t e) const
  {
    u_int64_t def_range = 10;
    u_int64_t l = 0;
    u_int64_t h = def_range;
    bool loud;

    loud = e.set_loud (true);
    if (_low) {
      l = _low->eval_as_uint (e);
    }
    h = _high->eval_as_uint (e);
    e.set_loud (loud);

    int64_t d = h - l;

    if (d <= 0) {
      strbuf b ("range for random must be greater than 0 (got %" PRId64 ")", d);
      report_error (e, b);
      h = l + def_range;
    }

    int64_t range = h - l;
    assert (range > 0);

    u_int64_t v = (random () % range) + l;
    scalar_obj_t o;
    o.set_u (v);
    return o;
  }

  //-----------------------------------------------------------------------


};
