
#include "okrfn-int.h"
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

  const str rand_t::DOCUMENTATION =
    "Given one argument (//a1//), outputs an integer randomly "
    "between 0 and //a1//.  Given two (a1 and a1), outputs an integer randomly "
    "between //a1// and //a2//";
  //-----------------------------------------------------------------------

  ptr<const expr_t>
  cmp_float_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double d = args[0]._f;
    int ret = 0;
    if (d > 0) { ret = 1; }
    else if (d < 0) { ret = -1; }
    return expr_int_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  round_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double d = args[0]._f;
    return expr_double_t::alloc (round (d));
  }

  //-----------------------------------------------------------------------

  const str round_t::DOCUMENTATION =
    "Round the input float //f// to the nearest integer.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  exp_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double e = args[0]._f;
    return expr_double_t::alloc (exp (e));
  }

  //-----------------------------------------------------------------------

  const str exp_t::DOCUMENTATION =
    "The C-library exp() function.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  log_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double b = args[0]._f;
    return expr_double_t::alloc (log (b));
  }

  //-----------------------------------------------------------------------

  const str log_t::DOCUMENTATION =
    "The C-library log() function.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  sqrt_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (sqrt (n));
  }

  //-----------------------------------------------------------------------

  const str sqrt_t::DOCUMENTATION =
    "The C-library sqrt() function.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  cos_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (cos (n));
  }

  //-----------------------------------------------------------------------

  const str cos_t::DOCUMENTATION =
    "The C-library cos() function, input in radians.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  tan_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (tan (n));
  }

  //-----------------------------------------------------------------------

  const str tan_t::DOCUMENTATION =
    "The C-library tan() function, input in radians.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  asin_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (asin (n));
  }

  //-----------------------------------------------------------------------

  const str asin_t::DOCUMENTATION =
    "The C-library asin() function, input in radians.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  acos_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (acos (n));
  }

  //-----------------------------------------------------------------------

  const str acos_t::DOCUMENTATION =
    "The C-library acos() function, input in radians.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  sin_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (sin (n));
  }

  //-----------------------------------------------------------------------

  const str sin_t::DOCUMENTATION =
    "The C-library sin() function, input in radians.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  atan_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double n = args[0]._f;
    return expr_double_t::alloc (atan (n));
  }

  //-----------------------------------------------------------------------

  const str atan_t::DOCUMENTATION =
    "The C-library atan() function, input in radians.";


  //-----------------------------------------------------------------------

  ptr<const expr_t>
  atan2_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double y = args[0]._f;
    double x = args[1]._f;
    return expr_double_t::alloc (atan2 (y,x));
  }

  //-----------------------------------------------------------------------

  const str atan2_t::DOCUMENTATION =
    "The C-library atan2() function, input in radians.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  pow_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double b = args[0]._f;
    double e = args[1]._f;
    return expr_double_t::alloc (pow (b, e));
  }

  //-----------------------------------------------------------------------

  const str pow_t::DOCUMENTATION =
    "The C-library pow() function.";


  //-----------------------------------------------------------------------

  ptr<const expr_t>
  ceil_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double d = args[0]._f;
    return expr_double_t::alloc (ceil (d));
  }

  //-----------------------------------------------------------------------

  const str ceil_t::DOCUMENTATION =
    "Return the smallest integer //i// such that //i >= f//.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  floor_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    double d = args[0]._f;
    return expr_double_t::alloc (floor (d));
  }

  //-----------------------------------------------------------------------

  const str floor_t::DOCUMENTATION =
    "Return the largest integer //i// such that //i <= f//.";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  bitwise_xor_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    u_int64_t l = args[0]._u;
    u_int64_t r = args[1]._u;
    return expr_uint_t::alloc (l ^ r);
  }


  //-----------------------------------------------------------------------

  ptr<const expr_t>
  bitwise_leftshift_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    u_int64_t base = args[0]._u;
    u_int64_t shift = args[1]._u;
    return expr_uint_t::alloc (base << shift);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  bitwise_rightshift_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    u_int64_t base = args[0]._u;
    u_int64_t shift = args[1]._u;
    return expr_uint_t::alloc (base >> shift);
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
	  report_error (p, strbuf ("argument %zu to bitwise_or() is null", 
				   hi));
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

  const str bitwise_or_t::DOCUMENTATION =
    "Compute the bitwise OR of the two arguments. Can take "
    "arbitrarily many arguments, but only two are shown here.";

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

  const str bitwise_and_t::DOCUMENTATION =
    "Compute the bitwise AND of the two arguments. Can take "
    "arbitrarily many arguments, but only two are shown here.";

  //-----------------------------------------------------------------------

};
