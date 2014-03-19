
#include "okrfn-int.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  const char *libname = "rfn3";

  //-----------------------------------------------------------------------

  lib_t::lib_t () : library_t ()
  {

#define F(f) \
  _functions.push_back (New refcounted<f##_t> ())

    F(rand);
    F(len);
    F(type);
    F(join);
    F(append);
    F(range);
    F(split);
    F(split2);
    F(map);
    F(search);
    F(match);
    F(tolower);
    F(toupper);
    F(replace);
    F(html_escape);
    F(tag_escape);
    F(json_escape);
    F(js_escape);
    F(hidden_escape);
    F(substr);
    F(strip);
    F(url_escape);
    F(url_unescape);
    F(sha1);
    F(import);
    F(version_str);
    F(version_int);
    F(okws_version_str);
    F(okws_version_int);
    F(values);
    F(keys);
    F(items);
    F(remove);
    F(json2pub);
    F(cgi2pub);
    F(now);
    F(days_from_now);
    F(time_from_now);
    F(time_format);
    F(localtime);
    F(localtime_raw);
    F(mktime);
    F(strptime);
    F(decorate);
    F(isnull);
    F(default);
    F(enumerate);
    F(dump_env);
    F(contains);
    F(int);
    F(uint);
    F(round);
    F(cmp_float);
    F(ceil);
    F(floor);
    F(bitwise_or);
    F(bitwise_and);
    F(bitwise_xor);
    F(bitwise_leftshift);
    F(bitwise_rightshift);
    F(str);
    F(documentation);
    F(logwarn);
    F(warn);
    F(warn_trace);
    F(enable_wss);
    F(internal_dump);
    F(bind);
    F(unbind);
    F(copy);
    F(lookup);
    F(pop_front);
    F(list);
    F(sort);
    F(sort2);
    F(reverse);
    F(shuffle);
    F(randsel);
    F(cmp);
    F(format_float);
    F(format_int);
    F(stat_file);
    F(raw);
    F(utf8_fix);
    F(wss_filter);
    F(fork);
    F(sleep);
    F(shotgun);
    F(log);
    F(exp);
    F(pow);
    F(sqrt);
    F(splice);
    F(slice);
    F(reserve);
    F(index_of);
    F(eval_location);
    F(breadcrumb);
    F(stacktrace);
    F(cos);
    F(sin);
    F(tan);
    F(asin);
    F(acos);
    F(atan);
    F(atan2);

#undef F

    // Bind the whole library into "rfn3.foo"
    bind_all (libname);
  }

  //-----------------------------------------------------------------------

  ptr<lib_t> lib_t::alloc () { return New refcounted<lib_t> (); }

  //-----------------------------------------------------------------------

  const char *version_str () { return "3.0.20"; }
  u_int64_t version_int () { return 3000020; }

  //-----------------------------------------------------------------------
};


