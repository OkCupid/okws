
#include "okrfn.h"
#include "okformat.h"
#include <math.h>

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  rand_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
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

  ptr<const expr_t>
  round_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double d = args[0]._f;
    return expr_double_t::alloc (round (d));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  exp_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double e = args[0]._f;
    return expr_double_t::alloc (exp (e));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  log_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double b = args[0]._f;
    return expr_double_t::alloc (log (b));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  sqrt_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (sqrt (n));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  pow_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double b = args[0]._f;
    double e = args[1]._f;
    return expr_double_t::alloc (pow (b, e));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  ceil_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double d = args[0]._f;
    return expr_double_t::alloc (ceil (d));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  floor_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double d = args[0]._f;
    return expr_double_t::alloc (floor (d));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  bitwise_or_t::v_eval_1 (eval_t *p, const margs_t &args) const
  {
    vec<u_int64_t> v;
    ptr<const expr_t> ret;
    u_int64_t y = 0;
    bool ok = true;
    ptr<expr_t> x;

    if (args.size () < 2) {
      report_error (p, "bitwise_or() takes 2 or more arguments");
      ok = false;
    } else {
      for (size_t i = 0; i < args.size (); i++) {
	size_t hi = i + 1;
	u_int64_t u;
	if (!(x = args[i])) {
	  report_error (p, strbuf ("argument %zu to bitwise_or() is null", hi));
	  ok = false;
	} else if (!x->to_uint (&u)) {
	  report_error (p, strbuf ("argument %zu to bitwise_or() is not a "
				   "positive int", hi));
	  ok = false;
	} else {
	  y = y | u;
	}
      }
    }

    if (ok) {
      ret = expr_uint_t::alloc (y);
    } else {
      ret = expr_null_t::alloc ();
    }

    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  bitwise_and_t::v_eval_1 (eval_t *p, const margs_t &args) const
  {
    vec<u_int64_t> v;
    ptr<const expr_t> ret;
    u_int64_t y = UINT64_MAX;
    bool ok = true;
    ptr<expr_t> x;
    
    if (args.size () < 2) {
      report_error (p, "bitwise_and() takes 2 or more arguments");
      ok = false;
    } else {
      for (size_t i = 0; i < args.size (); i++) {
	size_t hi = i + 1;
	u_int64_t u;
	if (!(x = args[i])) {
	  strbuf b ("argument %zu to bitwise_and() is null", hi);
	  report_error (p, b);
	  ok = false;
	} else if (!x->to_uint (&u)) {
	  report_error (p, strbuf ("argument %zu to bitwise_and() is not a "
				   "positive int", hi));
	  ok = false;
	} else {
	  y = y & u;
	}
      }
    }
    
    if (ok) {
      ret = expr_uint_t::alloc (y);
    } else {
      ret = expr_null_t::alloc ();
    }
    return ret;
  }
  
  //-----------------------------------------------------------------------

};
