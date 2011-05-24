// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#pragma once

#include "pub3.h"
#include "qhash.h"
#include "pub3lib.h"
#include "okrfn.h"

//=======================================================================

namespace rfn3 {

  //-----------------------------------------------------------------------

  using namespace pub3;
  extern const char *libname;

  //-----------------------------------------------------------------------

  PUB3_COMPILED_FN(rand, "|uu");
  PUB3_COMPILED_FN(len, "O");
  PUB3_COMPILED_FN(join, "sl");
  PUB3_COMPILED_FN(range, "i|ii");
  PUB3_COMPILED_FN(split, "rs");
  PUB3_COMPILED_FN(split2, "rs");
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
  PUB3_COMPILED_FN(time_format, "us");
  PUB3_COMPILED_FN(localtime, "|u");
  PUB3_COMPILED_FN(days_from_now, "i");
  PUB3_COMPILED_FN(time_from_now, "|iiii"); // D,H,M,S args
  PUB3_COMPILED_FN(strptime, "s|s"); 
  PUB3_COMPILED_FN(decorate, "l");
  PUB3_COMPILED_FN(enumerate, "l");
  PUB3_COMPILED_FN(dump_env, "");
  PUB3_COMPILED_FN(contains, "lO");
  PUB3_COMPILED_FN(int, "O");
  PUB3_COMPILED_FN(round, "f");
  PUB3_COMPILED_FN(ceil, "f");
  PUB3_COMPILED_FN(floor, "f");
  PUB3_COMPILED_FN(pow, "ff");
  PUB3_COMPILED_FN(sqrt, "f");
  PUB3_COMPILED_FN(exp, "f");
  PUB3_COMPILED_FN(log, "f");
  PUB3_COMPILED_FN(str, "O");
  PUB3_COMPILED_FN(tag_escape, "s|r");
  PUB3_COMPILED_FN(replace, "srO|b");
  PUB3_COMPILED_FN(warn, "s");
  PUB3_COMPILED_FN(logwarn, "s|b");
  PUB3_COMPILED_FN(warn_trace, "s");
  PUB3_COMPILED_FN(enable_wss, "b");
  PUB3_COMPILED_FN(internal_dump, "O");
  PUB3_COMPILED_FN(unbind, "s|s");
  PUB3_COMPILED_FN(copy, "O");
  PUB3_COMPILED_FN(bind, "sO|s");
  PUB3_COMPILED_FN(list, "O");
  PUB3_COMPILED_FN(sort, "l|F");
  PUB3_COMPILED_FN(cmp, "OO");
  PUB3_COMPILED_FN(format_float, "sf");
  PUB3_COMPILED_FN(format_int, "si");
  PUB3_COMPILED_FN(json_escape, "s");
  PUB3_COMPILED_FN(randsel, "l");
  PUB3_COMPILED_FN(bitwise_leftshift, "uu");
  PUB3_COMPILED_FN(bitwise_rightshift, "uu");
  PUB3_COMPILED_FN(bitwise_xor, "uu");
  PUB3_COMPILED_FN(index_of, "ss|i");

  PUB3_FILTER(utf8_fix);
  PUB3_FILTER(strip);
  PUB3_FILTER(url_escape);
  PUB3_FILTER(url_unescape);
  PUB3_FILTER(sha1);
  PUB3_FILTER(tolower);
  PUB3_FILTER(toupper);
  PUB3_FILTER(html_escape);
  PUB3_FILTER(hidden_escape);
  PUB3_FILTER(wss_filter);
  
  //-----------------------------------------------------------------------

  PUB3_COMPILED_FN_BLOCKING(sleep, "i|i");
  PUB3_COMPILED_FN(fork, "F");
  PUB3_COMPILED_FN_BLOCKING(shotgun, "l");

  //-----------------------------------------------------------------------

  PUB3_COMPILED_HANDROLLED_FN(isnull);
  PUB3_COMPILED_UNPATTERNED_FN(append);
  PUB3_COMPILED_UNPATTERNED_FN(splice);
  PUB3_COMPILED_UNPATTERNED_FN(shuffle);
  PUB3_COMPILED_UNPATTERNED_FN(reverse);
  PUB3_COMPILED_HANDROLLED_FN(default);
  PUB3_COMPILED_UNPATTERNED_FN(bitwise_or);
  PUB3_COMPILED_UNPATTERNED_FN(bitwise_and);
  
  //-----------------------------------------------------------------------

  class map_t : public patterned_fn_t {
  public:
    map_t () : patterned_fn_t (libname, "map", "dO") {}
    ptr<const expr_t> v_eval_2 (eval_t *p, const vec<arg_t> &args) const;
  protected:
    ptr<expr_t> eval_internal (eval_t *p, ptr<const expr_dict_t> m, 
			       ptr<const expr_t> x) const;
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
    ptr<const expr_t> v_eval_2 (eval_t *e, const vec<arg_t> &args) const;
    bool safe_args () const { return false; }
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
