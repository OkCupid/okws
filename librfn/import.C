#include "okrfnlib.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  import_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_dict_t> d = args[0]._d;
    ptr<expr_dict_t> c = d->copy ();
    bool res = true;
    if (!p->env ()) {
      report_error (e, "empty env in import()");
      res = false;
    } else {
      p->env ()->push_locals (c, false);
    }
    return expr_bool_t::alloc (res);
  }

  //-----------------------------------------------------------------------

};
