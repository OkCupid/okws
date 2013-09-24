#include "okrfn-int.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  ptr<const expr_t> 
  now_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  { 
    time_t t = sfs_get_timenow ();
    return expr_uint_t::alloc (t);
  }

  //-----------------------------------------------------------------------

  const str now_t::DOCUMENTATION =
    "Output the time now in Unix-timestamp";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  time_format_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    str fmt = args[1]._s;
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
    } else if (!strftime (buf, BUFSZ, fmt.cstr(), &stm)) {
      report_error (p, strbuf ("format '%s' failed", fmt.cstr ()));
    } else {
      ret = buf;
    }
    return expr_str_t::alloc (ret);
  }

  //-----------------------------------------------------------------------

  const str time_format_t::DOCUMENTATION =
    "Like strftime, format the Unix timestamp //time// according to the format "
    "//fmt//";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  localtime_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    time_t t = 0;
    if (args.size () > 0) { t = args[0]._u; }
    if (!t) { t = sfs_get_timenow (); }
    ptr<expr_t> ret;

    struct tm stm;
    if (!localtime_r (&t, &stm)) {
      report_error (p, strbuf ("cannot convert '") << t << "' to time");
    } else {
      ptr<expr_list_t> l = expr_list_t::alloc ();
#define F(i) \
      l->push_back (expr_int_t::alloc (i));
      F(stm.tm_year + 1900);
      F(stm.tm_mon + 1);
      F(stm.tm_mday);
      F(stm.tm_hour);
      F(stm.tm_min);
      F(stm.tm_sec);
      F(stm.tm_wday);
      F(stm.tm_yday);
#undef F
      ret = l;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  localtime_raw_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    time_t t = 0;
    if (args.size () > 0) { t = args[0]._u; }
    if (!t) { t = sfs_get_timenow (); }
    ptr<expr_t> ret;

    struct tm stm;
    if (!localtime_r (&t, &stm)) {
      report_error (p, strbuf ("cannot convert '") << t << "' to time");
    } else {
      ptr<expr_dict_t> d = expr_dict_t::alloc ();
#define F(i) \
      d->insert(#i, (int64_t)stm.tm_##i)
      F(sec);
      F(min);
      F(hour);
      F(mday);
      F(mon);
      F(year);
      F(wday);
      F(yday);
      F(isdst);
#undef F
      ret = d;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  mktime_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    struct tm t = {0};

    const expr_dict_t& d = *args[0]._d;
    ptr <const expr_t> e;

    if ((e = d.lookup("sec")))   e->to_str().to_int32(&t.tm_sec);
    if ((e = d.lookup("min")))   e->to_str().to_int32(&t.tm_min);
    if ((e = d.lookup("hour")))  e->to_str().to_int32(&t.tm_hour);
    if ((e = d.lookup("mday")))  e->to_str().to_int32(&t.tm_mday);
    if ((e = d.lookup("mon")))   e->to_str().to_int32(&t.tm_mon);
    if ((e = d.lookup("year")))  e->to_str().to_int32(&t.tm_year);
    if ((e = d.lookup("wday")))  e->to_str().to_int32(&t.tm_wday);
    if ((e = d.lookup("yday")))  e->to_str().to_int32(&t.tm_yday);
    if ((e = d.lookup("isdst"))) e->to_str().to_int32(&t.tm_isdst);

    return expr_int_t::alloc(mktime(&t));
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  days_from_now_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    u_int64_t u = sfs_get_timenow () + 60 * 60 * 24 * args[0]._i;
    return expr_uint_t::alloc (u);
  }

  //-----------------------------------------------------------------------

  const str days_from_now_t::DOCUMENTATION =
    "Return the Unix timestamp of //days// days from now";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  time_from_now_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
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

  const str time_from_now_t::DOCUMENTATION =
    "Output the Unix timestamp at //d// days, //h// hours, "
    "//m// minutes and //s// seconds from now";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  strptime_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
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
      report_error (p, strbuf ("strptime(\"%s\", \"%s\") failed", t, f));
      ret = expr_null_t::alloc ();
    } else {
      stm.tm_isdst = -1;
      ret = expr_int_t::alloc (mktime (&stm));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  const str strptime_t::DOCUMENTATION =
    "Read from the string //time//, formatted "
    "according to the format string //fmt//, into a UNIX timestamp. "
    "If no //fmt// is given, assume '%F', which means 'YYYY-MM-DD'";

  //-----------------------------------------------------------------------

};
