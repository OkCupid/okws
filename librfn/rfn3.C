
#include "okrfn.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  lib_t::lib_t () : library_t ()
  {

#define F(f) \
  _functions.push_back (New refcounted<f##_t> ())

    F(random);
    F(len);
    F(type);
    F(join);
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
    F(substring);
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
    F(is_null);
    F(default);

#undef F
  }

  //-----------------------------------------------------------------------

  ptr<lib_t> lib_t::alloc () {  return New refcounted<lib_t> (); }

  //-----------------------------------------------------------------------
};


