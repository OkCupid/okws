#include "okrfn.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t> 
  now_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  { 
    time_t t = sfs_get_timenow ();
    return expr_uint_t::alloc (t);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  time_format_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str fmt = args[0]._s;
    time_t t;
    t = args[0]._u;
    if (!t) { t = sfs_get_timenow (); }
      
    enum { BUFSZ = 1024 };
    char buf[BUFSZ];

    struct tm stm;
    str ret ("<time-error>");

    if (!localtime_r (&t, &stm)) {
      report_error (p, strbuf ("cannot convert '") << t << "' to time");
    } else if (!fmt || fmt.len () == 0) {
      report_error (p, "bad time format given");
    } else if (!strftime (buf, BUFSZ, fmt, &stm)) {
      report_error (p, strbuf ("format '%s' failed", fmt.cstr ()));
    } else {
      ret = buf;
    }
    return expr_str_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  days_from_now_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    u_int64_t u = sfs_get_timenow () + 60 * 60 * 24 * args[0]._i;
    return expr_uint_t::alloc (u);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  time_from_now_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    scalar_obj_t so; 
    time_t t = sfs_get_timenow ();
    int delta = 0;
    int multiples[] = { 1, 24, 60, 60, 0 };
    size_t i = 0;

    for (int *mp = multiples; *mp; mp++, i++) {
      delta *= *mp;
      if (i < args.size ()) { delta += args[i]._i; }
    }

    u_int64_t ret = delta + t;
    return expr_uint_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  strptime_t::v_eval_2 (publish_t *p, const vec<arg_t> &args) const
  {
    str timestr = args[0]._s;
    str fmt = "%F"; // YYYY-MM-DD
    struct tm stm;
    if (args.size () > 1) { fmt = args[1]._s; }
    memset ((void *)&stm, 0, sizeof (stm));

    const char *t = timestr.cstr ();
    const char *f = fmt.cstr ();

    ptr<expr_t> ret;
    if (!strptime (t, f, &stm)) {
      report_error (p, strbuf ("strptime(%s,%s) failed", t,f));
      ret = expr_null_t::alloc ();
    } else {
      ret = expr_int_t::alloc (mktime (&stm));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

};
