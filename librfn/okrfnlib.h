// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#pragma once

#include "pub3.h"
#include "qhash.h"
#include "okrfn.h"

namespace rfn3 {
  
#define PUB3_COMPILED_FN(x,pat)				                \ 
  class x##_t : public patterned_fn_t {                                 \ 
  public:								\
    x##_t () : patterned_fn_t (#x, pat) {}				\
    ptr<const expr_t>							\
    v_eval_2 (publish_t *p, const vec<arg_t> &args) const;		\ 
  }

#define PUB3_COMPILED_HANDROLLED_FN(x)					\
    class x##_t : public compiled_handrolled_fn_t {			\
    public:								\
    x##_t () : compiled_handrolled_fn_t (#x) {}				\
    void pub_to_val (publish_t *p, args_t args, cxev_t, CLOSURE) const;	\
    }
  
  using namespace pub3;

  PUB3_COMPILED_FN(random, "|uu");
  PUB3_COMPILED_FN(len, "O");
  PUB3_COMPILED_FN(type, "O");
  PUB3_COMPILED_FN(join, "sl",);
  PUB3_COMPILED_FN(range, "i|ii");
  PUB3_COMPILED_FN(split, "rs");
  PUB3_COMPILED_FN(map, "dl");
  PUB3_COMPILED_FN(search, "rs");
  PUB3_COMPILED_FN(match, "rs");
  PUB3_COMPILED_FN(tolower, "s");
  PUB3_COMPILED_FN(toupper, "s");
  PUB3_COMPILED_FN(html_escape, "s");
  PUB3_COMPILED_FN(tag_escape, "s");
  PUB3_COMPILED_FN(json_escape, "s");
  PUB3_COMPILED_FN(hidden_escape, "s");
  PUB3_COMPILED_FN(substring, "si|i");
  PUB3_COMPILED_FN(strip, "s");
  PUB3_COMPILED_FN(url_escape, "s");
  PUB3_COMPILED_FN(url_unescape, "s");
  PUB3_COMPILED_FN(sha1, "s");
  PUB3_COMPILED_FN(import, "d");
  PUB3_COMPILED_FN(version_str, "");
  PUB3_COMPILED_FN(version_int, "");
  PUB3_COMPILED_FN(okws_version_str, "");
  PUB3_COMPILED_FN(okws_version_int, "");
  PUB3_COMPILED_FN(values, "d");
  PUB3_COMPILED_FN(keys, "d");
  PUB3_COMPILED_FN(items, "d");
  PUB3_COMPILED_FN(json2pub, "s");

  PUB3_COMPILED_HANDROLLED_FN(is_null);
  PUB3_COMPILED_HANDROLLED_FN(default);
  PUB3_COMPILED_HANDROLLED_FN(append);
  
  //-----------------------------------------------------------------------

};

#endif /* _LIBRFN_OKRFNLIB_H_ */
