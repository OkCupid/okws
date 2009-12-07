// -*-c++-*-
/* $Id: web.h 4029 2009-01-30 13:28:14Z max $ */

#pragma once

#include "pub3.h"
#include "qhash.h"

namespace rfn3 {

  using namespace pub3;

  //-----------------------------------------------------------------------

  const char *version_str ();
  const char *okws_version_str ();
  u_int64_t version_int ();
  u_int64_t okws_version_int ();

  //-----------------------------------------------------------------------

  class compiled_fn_t : public expr_t, public callable_t {
  public:
    compiled_fn_t (str n);
    virtual ~compiled_fn_t () {}
    void set_lib (str l);
    
    virtual ptr<const expr_t> eval_to_val (publish_t *e, args_t args) const;
    virtual void pub_to_val (publish_t *p, args_t args, cxev_t, CLOSURE) const;
    str to_str () const;

  protected:
    // evaluate, given that the arguments have been prevaluted...
    virtual ptr<const expr_t> v_eval_1 (publish_t *e, args_t args) const = 0;

    ptr<const callable_t> to_callable () const { return mkref (this); }
    const char *get_obj_name () const { return "rfn1::runtime_fn_t"; }

    virtual bool check_args (publish_t *p, args_t a) const;
    virtual size_t min_args () const { return 0; }
    virtual size_t max_args () const { return 0; }

    void pub_args (publish_t *p, args_t in, event<args_t>::ref ev, CLOSURE) 
      const;
    args_t eval_args (publish_t *p, args_t in) const;

    str _name, _lib;
  };

  //-----------------------------------------------------------------------

  class patterned_fn_t : public compiled_fn_t {
  public:
    patterned_fn_t (str n, str p) : compiled_fn_t (n), _arg_pat (p) {}
    virtual bool might_block () const { return false; }
    
  protected:

    struct arg_t {
      ptr<expr_t> _O;
      int64_t _i;
      u_int64_t _u;
      bool _b;
      ptr<regex_t> _r;
      str _s;
      ptr<expr_dict_t> _d;
      ptr<expr_list_t> _l;
    };

    // evaluate, given that the arguments have been prevaluted...
    ptr<const expr_t> v_eval_1 (publish_t *e, args_t args) const;

    // evaluate, given that the args have been preevaluated and type-checked
    virtual ptr<const expr_t> v_eval_2 (publish_t *e, args_t args) const = 0;

    virtual bool check_args (publish_t *p, args_t a) const;

    str _arg_pat;
  };

  //-----------------------------------------------------------------------
};

#endif /* _LIBRFN_OKRFN_H_ */
