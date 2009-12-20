#include "okrfn.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t> 
  regex_fn_t::v_eval_1 (publish_t *e, const cargs_t &args) const
  {
    ptr<rxx> my_rxx;
    str target;
    ptr<const expr_t> ret;
    bool ok = true;

    if (args.size () == 2) {
      my_rxx = args[0]->to_regex ();
      target = args[1]->to_str ();
    } else if (args.size () == 3) {
      str b = args[0]->to_str ();
      str opts = args[1]->to_str ();
      target = args[2]->to_str ();
      if (b) {
	my_rxx = str2rxx (e, b, opts);
      }
    } else {
      ok = false;
      strbuf prob ("expected 2-3 arguments to %s; got %zu",
		   _name.cstr (), args.size ());
      report_error (e, prob);
    }

    if (!ok) {
      /* noop */

    } else if (!my_rxx) {
      str b = args[0]->to_str ();
      
      if (b) { 
	report_error (e, strbuf ("failed to compile regex: %s", b.cstr ()));
      } else {
	report_error (e, "null regex given");
      }

    } else if (!target) {
      report_error (e, "null target given for regex operation");

    } else {
      bool b = match() ? my_rxx->match (target) : my_rxx->search (target);
      ret = expr_bool_t::alloc (b);
    }
    return ret;
  }
  
  //-----------------------------------------------------------------------

};
