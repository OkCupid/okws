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

  const str now_t::DOCUMENTATION = R"*(Output the time now in Unix-timestamp

@return {uint}
@example now())*";

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
    R"*(Like `strftime`, format the Unix timestamp `time` according to the format
`fmt`

@param {uint} time
@param {string} fmt
@return {string}
@example time_format (now(), '%a, %d %b %Y %H:%M:%S GMT'))*";

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

  const str localtime_t::DOCUMENTATION = R"*(Convert timestamp to list of time
component values.

Returned list has the order:
`[year, month, day, hour, min, sec, weekday, yearday]`

Weekday is the number of days since Sunday.
Yearday is the number of days since January 1.

@param {int} timestamp
@return {list}
@example localtime(now()))*";

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

  const str localtime_raw_t::DOCUMENTATION = R"*(Convert timestamp to dict
of component values.

Uses the value of `timestamp` to fill a tm dict with the values that represent
the corresponding time, expressed for the local timezone
@param {int} timestamp
@return {dict}
@example localtime_raw(now()))*";

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

  const str mktime_t::DOCUMENTATION = R"*(Returns a timestamp that represents the
local time described by the `tm` dict (which may be modified).

This function performs the reverse translation that `localtime_raw` does.

@param {dict} tm
@return {int}
@example
out_tm = localtime_raw(now());
out_tm.mon = out_tm.mon + 3;
expiration = mktime(out_tm);
@response)*";

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  days_from_now_t::v_eval_2 (eval_t *p, const vec<arg_t> &args) const
  {
    u_int64_t u = sfs_get_timenow () + 60 * 60 * 24 * args[0]._i;
    return expr_uint_t::alloc (u);
  }

  //-----------------------------------------------------------------------

  const str days_from_now_t::DOCUMENTATION = R"*(Return the Unix timestamp of
`days` days from now

@param {int} days
@return {uint})*";

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

  const str time_from_now_t::DOCUMENTATION = R"*(Output the Unix timestamp at `d`
days, `h` hours, `m` minutes and `s` seconds from now

@optional
@param {int} d
@param {int} h
@param {int} m
@param {int} s
@return {uint})*";

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
      ret = expr_null_t::alloc ();
    } else {
      stm.tm_isdst = -1;
      ret = expr_int_t::alloc (mktime (&stm));
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  const str strptime_t::DOCUMENTATION = R"*(Read from the string `time`, formatted
according to the format string `fmt`, into a UNIX timestamp. 
Returns null for an invalid `time`.

If no `fmt` is given, assume `'%F'`, which means `'YYYY-MM-DD'`

@param {string} s
@optional
@param {string} fmt
@return {string})*";

  //-----------------------------------------------------------------------

};
