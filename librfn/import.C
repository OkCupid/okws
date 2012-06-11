#include "okrfn-int.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  import_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    ptr<const expr_dict_t> d = args[0]._d;
    ptr<expr_dict_t> c = expr_dict_t::safe_copy (d);
    bool res = true;
    if (!p->env ()) {
      report_error (p, "empty env in import()");
      res = false;
    } else if (c->size ()) {
      p->env ()->push_locals (c);
    }
    return expr_bool_t::alloc (res);
  }

  //-----------------------------------------------------------------------

  const str import_t::DOCUMENTATION =
    "Take the given dictionary and import it into the environment "
    "as local bindings.";

  //-----------------------------------------------------------------------
};
