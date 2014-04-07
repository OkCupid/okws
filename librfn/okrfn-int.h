// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#pragma once

#include "pub3.h"
#include "qhash.h"
#include "str.h"
#include "pub3lib.h"
#include "okrfn.h"

//=======================================================================

namespace rfn3 {

  //-----------------------------------------------------------------------

  PUB3_COMPILED_FN_DOC(rand, "|uu");
  PUB3_COMPILED_FN_DOC(len, "O");
  PUB3_COMPILED_FN_DOC(join, "sl");
  PUB3_COMPILED_FN_DOC(range, "i|ii");
  PUB3_COMPILED_FN_DOC(split, "rs");
  PUB3_COMPILED_FN(split2, "rs");
  PUB3_COMPILED_FN_DOC(substr, "si|i");
  PUB3_COMPILED_FN_DOC(import, "d");
  PUB3_COMPILED_FN_DOC(version_str, "");
  PUB3_COMPILED_FN_DOC(version_int, "");
  PUB3_COMPILED_FN_DOC(okws_version_str, "");
  PUB3_COMPILED_FN_DOC(okws_version_int, "");
  PUB3_COMPILED_FN_DOC(values, "d");
  PUB3_COMPILED_FN_DOC(keys, "d");
  PUB3_COMPILED_FN_DOC(items, "d");
  PUB3_COMPILED_FN(remove, "ds");
  PUB3_COMPILED_FN_DOC(json2pub, "s|b");
  PUB3_COMPILED_FN_DOC(cgi2pub, "s");
  PUB3_COMPILED_FN_DOC(now, "");
  PUB3_COMPILED_FN_DOC(time_format, "us");
  PUB3_COMPILED_FN(localtime, "|u");
  PUB3_COMPILED_FN(localtime_raw, "|u");
  PUB3_COMPILED_FN(mktime, "d");
  PUB3_COMPILED_FN_DOC(days_from_now, "i");
  PUB3_COMPILED_FN_DOC(time_from_now, "|iiii"); // D,H,M,S args
  PUB3_COMPILED_FN_DOC(strptime, "s|s"); 
  PUB3_COMPILED_FN_DOC(decorate, "l");
  PUB3_COMPILED_FN_DOC(enumerate, "l");
  PUB3_COMPILED_FN_DOC(dump_env, "");
  PUB3_COMPILED_FN_DOC(contains, "lO");
  PUB3_COMPILED_FN_DOC(int, "O");
  PUB3_COMPILED_FN(uint, "O");
  PUB3_COMPILED_FN_DOC(round, "f");
  PUB3_COMPILED_FN_DOC(ceil, "f");
  PUB3_COMPILED_FN_DOC(floor, "f");
  PUB3_COMPILED_FN_DOC(pow, "ff");
  PUB3_COMPILED_FN_DOC(sqrt, "f");
  PUB3_COMPILED_FN_DOC(exp, "f");
  PUB3_COMPILED_FN_DOC(log, "f");
  PUB3_COMPILED_FN_DOC(sin, "f");
  PUB3_COMPILED_FN_DOC(cos, "f");
  PUB3_COMPILED_FN_DOC(tan, "f");
  PUB3_COMPILED_FN_DOC(asin, "f");
  PUB3_COMPILED_FN_DOC(acos, "f");
  PUB3_COMPILED_FN_DOC(atan, "f");
  PUB3_COMPILED_FN_DOC(atan2, "f");
  PUB3_COMPILED_FN_DOC(str, "O");
  PUB3_COMPILED_FN(documentation, "O");
  PUB3_COMPILED_FN_DOC(tag_escape, "s|r");
  PUB3_COMPILED_FN_DOC(logwarn, "s|b");
  PUB3_COMPILED_FN_DOC(warn_trace, "s");
  PUB3_COMPILED_FN_DOC(enable_wss, "b");
  PUB3_COMPILED_FN_DOC(internal_dump, "O");
  PUB3_COMPILED_FN_DOC(unbind, "s|s");
  PUB3_COMPILED_FN_DOC(copy, "O");
  PUB3_COMPILED_FN_DOC(bind, "sO|s");
  PUB3_COMPILED_FN_DOC(list, "O");
  PUB3_COMPILED_FN_DOC(sort, "l|F");
  PUB3_COMPILED_FN(sort2, "l|F");
  PUB3_COMPILED_FN_DOC(cmp, "OO");
  PUB3_COMPILED_FN_DOC(format_float, "sf");
  PUB3_COMPILED_FN_DOC(format_int, "si");
  PUB3_COMPILED_FN_DOC(json_escape, "s");
  PUB3_COMPILED_FN_DOC(js_escape, "s");
  PUB3_COMPILED_FN_DOC(randsel, "l");
  PUB3_COMPILED_FN(bitwise_leftshift, "uu");
  PUB3_COMPILED_FN(bitwise_rightshift, "uu");
  PUB3_COMPILED_FN(bitwise_xor, "uu");
  PUB3_COMPILED_FN(index_of, "ss|i");
  PUB3_COMPILED_FN(cmp_float, "f");
  PUB3_COMPILED_FN(eval_location, "");
  PUB3_COMPILED_FN(breadcrumb, "");
  PUB3_COMPILED_FN(stacktrace, "");
  PUB3_COMPILED_FN_DOC(slice, "l|ii");
  PUB3_COMPILED_FN_DOC(reserve, "lu");

  PUB3_FILTER_DOC(utf8_fix);
  PUB3_FILTER_DOC(strip);
  PUB3_FILTER_DOC(url_escape);
  PUB3_FILTER_DOC(url_unescape);
  PUB3_FILTER_DOC(sha1);
  PUB3_FILTER_DOC(tolower);
  PUB3_FILTER_DOC(toupper);
  PUB3_FILTER_DOC(html_escape);
  PUB3_FILTER_DOC(hidden_escape);
  PUB3_FILTER_DOC(wss_filter);
  
  //-----------------------------------------------------------------------

  PUB3_COMPILED_FN_BLOCKING_DOC(sleep, "i|i");
  PUB3_COMPILED_FN_DOC(fork, "F");
  PUB3_COMPILED_FN_BLOCKING_DOC(shotgun, "l");

  //-----------------------------------------------------------------------

  PUB3_COMPILED_HANDROLLED_FN_DOC(isnull);
  PUB3_COMPILED_UNPATTERNED_FN_DOC(append);
  PUB3_COMPILED_UNPATTERNED_FN_DOC(splice);
  PUB3_COMPILED_UNPATTERNED_FN_DOC(shuffle);
  PUB3_COMPILED_UNPATTERNED_FN_DOC(reverse);
  PUB3_COMPILED_HANDROLLED_FN_DOC(default);
  PUB3_COMPILED_UNPATTERNED_FN_DOC(bitwise_or);
  PUB3_COMPILED_UNPATTERNED_FN_DOC(bitwise_and);
  
  //-----------------------------------------------------------------------

  class map_t : public patterned_fn_t {
  public:
    map_t () : patterned_fn_t (libname, "map", "dO") {}
    ptr<const expr_t> v_eval_2 (eval_t *p, const vec<arg_t> &args) const;
  protected:
    ptr<expr_t> eval_internal (eval_t *p, ptr<const expr_dict_t> m, 
			       ptr<const expr_t> x) const;
    PUB3_DOC_MEMBERS
  };

  //-----------------------------------------------------------------------

  class regex_fn_t : public patterned_fn_t {
  public:
    regex_fn_t (str n) : patterned_fn_t (libname, n, "ss|s") {}
    ptr<const expr_t> v_eval_2 (eval_t *e, const vec<arg_t> &args) const;
    virtual bool match () const = 0;
  };
    
  //-----------------------------------------------------------------------
  
  class match_t : public regex_fn_t {
  public:
    match_t () : regex_fn_t ("match") {}
    bool match () const { return true; }
    PUB3_DOC_MEMBERS
  };
    
  //-----------------------------------------------------------------------

  class search_t : public regex_fn_t {
  public:
    search_t () : regex_fn_t ("search") {}
    bool match () const { return false; }
    PUB3_DOC_MEMBERS
  };

  //-----------------------------------------------------------------------

  class type_t : public patterned_fn_t {
  public:
    type_t () : patterned_fn_t (libname, "type", "O") {}
    ptr<const expr_t> v_eval_2 (eval_t *e, const vec<arg_t> &args) const;
    bool safe_args () const { return false; }
    PUB3_DOC_MEMBERS
  };

  //-----------------------------------------------------------------------

  class lookup_t : public compiled_fn_t {
  public:
    
    // Signature "s|s" -- key to lookup, and optional scope
    lookup_t () : compiled_fn_t (libname, "lookup") {}

    void pub_to_ref (eval_t *p, args_t args, mrev_t ev, CLOSURE) const;
    void pub_to_val (eval_t *p, args_t args, cxev_t ev, CLOSURE) const;
    ptr<mref_t> eval_to_ref (eval_t *p, args_t args) const;
    ptr<const expr_t> eval_to_val (eval_t *p, args_t args) const;
    PUB3_DOC_MEMBERS
  protected:
    bool count_args (eval_t *p, size_t s) const;
    ptr<mref_t> eval_final (eval_t *p, str k, str s) const;
  };

  //-----------------------------------------------------------------------

  class pop_front_t : public compiled_fn_t {
  public:
    // Signature "l" -- the list to pop from
    pop_front_t () : compiled_fn_t (libname, "pop_front") {}

    void pub_to_ref (eval_t *p, args_t args, mrev_t ev, CLOSURE) const;
    void pub_to_val (eval_t *p, args_t args, cxev_t ev, CLOSURE) const;
    void pub_to_mval (eval_t *p, args_t args, xev_t ev, CLOSURE) const;
    ptr<mref_t> eval_to_ref (eval_t *p, args_t args) const;
    ptr<const expr_t> eval_to_val (eval_t *p, args_t args) const;
    ptr<expr_t> eval_to_mval (eval_t *p, args_t args) const;
  protected:
    bool count_args (eval_t *p, size_t s) const;
    ptr<expr_t> eval_final (eval_t *p, ptr<expr_t> x) const;
  };

  //-----------------------------------------------------------------------

  class stat_file_t : public compiled_fn_t {
  public:
    // Signature "s" --- the filename to stat
    stat_file_t () : compiled_fn_t (libname, "stat_file") {}
    void pub_to_val (eval_t *p, args_t args, cxev_t ev, CLOSURE) const;
    ptr<const expr_t> eval_to_val (eval_t *p, args_t args) const;
    bool might_block () const { return true; }
  };

  //-----------------------------------------------------------------------

  class raw_t : public compiled_fn_t {
  public:
    // Signature "s" --- the file to read as raw
    raw_t () : compiled_fn_t (libname, "raw") {}
    void pub_to_val (eval_t *p, args_t args, cxev_t ev, CLOSURE) const;
    ptr<const expr_t> eval_to_val (eval_t *p, args_t args) const;
    bool might_block () const { return true; }
  };

  //-----------------------------------------------------------------------

  const char *version_str ();
  u_int64_t version_int ();
};

//=======================================================================
