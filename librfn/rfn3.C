
#include "okrfn.h"

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
    F(map);
    F(search);
    F(match);
    F(tolower);
    F(toupper);
    F(html_escape);
    F(tag_escape);
    F(json_escape);
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
    F(json2pub);
    F(now);
    F(days_from_now);
    F(time_from_now);
    F(strptime);
    F(decorate);
    F(isnull);
    F(default);
    F(enumerate);
    F(dump_env);
    F(contains);
    F(int);
    F(round);
    F(ceil);
    F(str);
    F(warn);
    F(warn_trace);

#undef F
  }

  //-----------------------------------------------------------------------

  ptr<lib_t> lib_t::alloc () {  return New refcounted<lib_t> (); }

  //-----------------------------------------------------------------------

  const char *version_str () { return "3.0.0"; }
  u_int64_t version_int () { return 3000000000ULL; }

  //-----------------------------------------------------------------------
};


