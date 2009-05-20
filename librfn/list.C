
#include "okrfn.h"

namespace rfn1 {

  //-----------------------------------------------------------------------

  join_t::join_t (const str &n, ptr<expr_list_t> el, int lineno)
    : scalar_fn_t (n, el, lineno),
      _join_str ((*el)[0]),
      _join_list ((*el)[1]) {}
  
  //-----------------------------------------------------------------------

  scalar_obj_t
  join_t::eval_internal (eval_t e) const
  {
    ptr<const vec_iface_t> l;
    str js;
    scalar_obj_t ret;

    if (!(l = _join_list->eval_as_vec (e))) {
      report_error (e, "cannot evaluate list argument to join");
    } else if (!(js = _join_str->eval_as_str (e))) {
      report_error (e, "cannot evaluate join string argument to join");
    } else {
      strbuf b;
      vec<str> hold;
      for (size_t i = 0; i < l->size (); i++) {
	if (i > 0) {
	  b << js;
	}
	str s;
	ptr<const pval_t> v = l->lookup (i);
	ptr<const expr_t> x;

	if (!v) {
	  /* nothing wanted, that's ok! */
	} else if (!(x = v->to_expr ())) {
	  strbuf err ("argument %zd in join list is not a pub3 obj!", i);
	  report_error (e, err);
	} else if (!(s = x->eval_as_str (e))) {
	  strbuf err ("cannot evaluate argument %zd in join list", i);
	  report_error (e, err);
	} else {
	  b << s;
	  hold.push_back (s);
	}
      }
      ret.set (str (b));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

};

