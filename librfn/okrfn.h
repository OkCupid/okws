// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#pragma once

#include "pub3.h"
#include "qhash.h"
#include "pub3lib.h"

namespace rfn3 {

  //-----------------------------------------------------------------------

  using namespace pub3;
  extern const char *libname;

  //-----------------------------------------------------------------------

  PUB3_COMPILED_FN(random, "|uu");
  PUB3_COMPILED_FN(len, "O");
  PUB3_COMPILED_FN(join, "sl");
  PUB3_COMPILED_FN(range, "i|ii");
  PUB3_COMPILED_FN(split, "rs");
  PUB3_COMPILED_FN(substr, "si|i");
  PUB3_COMPILED_FN(import, "d");
  PUB3_COMPILED_FN(version_str, "");
  PUB3_COMPILED_FN(version_int, "");
  PUB3_COMPILED_FN(okws_version_str, "");
  PUB3_COMPILED_FN(okws_version_int, "");
  PUB3_COMPILED_FN(values, "d");
  PUB3_COMPILED_FN(keys, "d");
  PUB3_COMPILED_FN(items, "d");
  PUB3_COMPILED_FN(json2pub, "s");
  PUB3_COMPILED_FN(now, "");
  PUB3_COMPILED_FN(time_format, "s|u");
  PUB3_COMPILED_FN(days_from_now, "i");
  PUB3_COMPILED_FN(time_from_now, "|iiii"); // D,H,M,S args
  PUB3_COMPILED_FN(strptime, "s|s"); 
  PUB3_COMPILED_FN(decorate, "l");
  PUB3_COMPILED_FN(enumerate, "l");
  PUB3_COMPILED_FN(dump_env, "");
  PUB3_COMPILED_FN(contains, "lO");
  PUB3_COMPILED_FN(int, "O");
  PUB3_COMPILED_FN(round, "d");
  PUB3_COMPILED_FN(ceil, "d");
  PUB3_COMPILED_FN(floor, "d");
  PUB3_COMPILED_FN(str, "O");
  PUB3_COMPILED_FN(tag_escape, "s|r");
  PUB3_COMPILED_FN(replace, "srs");
  PUB3_COMPILED_FN(warn, "s");
  PUB3_COMPILED_FN(warn_trace, "s");

  PUB3_FILTER(utf8_fix);
  PUB3_FILTER(strip);
  PUB3_FILTER(url_escape);
  PUB3_FILTER(url_unescape);
  PUB3_FILTER(sha1);
  PUB3_FILTER(tolower);
  PUB3_FILTER(toupper);
  PUB3_FILTER(html_escape);
  PUB3_FILTER(json_escape);
  PUB3_FILTER(hidden_escape);
  
  //-----------------------------------------------------------------------

  PUB3_COMPILED_HANDROLLED_FN(isnull);
  PUB3_COMPILED_HANDROLLED_FN(append);
  PUB3_COMPILED_HANDROLLED_FN(default);
  
  //-----------------------------------------------------------------------

  class map_t : public patterned_fn_t {
  public:
    map_t () : patterned_fn_t (libname, "map", "dO") {}
    ptr<const expr_t> v_eval_2 (publish_t *p, const vec<arg_t> &args) const;
  protected:
    ptr<expr_t> eval_internal (publish_t *p, ptr<const expr_dict_t> m, 
			       ptr<const expr_t> x) const;
  };

  //-----------------------------------------------------------------------

  class regex_fn_t : public patterned_fn_t {
  public:
    regex_fn_t (str n) : patterned_fn_t (libname, n, "ss|s") {}
    ptr<const expr_t> v_eval_2 (publish_t *e, const vec<arg_t> &args) const;
    virtual bool match () const = 0;
  };
    
  //-----------------------------------------------------------------------
  
  class match_t : public regex_fn_t {
  public:
    match_t () : regex_fn_t ("match") {}
    bool match () const { return true; }
  };
    
  //-----------------------------------------------------------------------

  class search_t : public regex_fn_t {
  public:
    search_t () : regex_fn_t ("search") {}
    bool match () const { return false; }
  };

  //-----------------------------------------------------------------------

  class type_t : public patterned_fn_t {
  public:
    type_t () : patterned_fn_t (libname, "type", "O") {}
    ptr<const expr_t> v_eval_2 (publish_t *e, const vec<arg_t> &args) const;
    bool safe_args () const { return false; }
  };

  //-----------------------------------------------------------------------

  const char *version_str ();
  u_int64_t version_int ();

  //-----------------------------------------------------------------------
  
  class lib_t : public pub3::library_t {
  public:
    lib_t ();
    static ptr<lib_t> alloc ();
  };
};

//=======================================================================
