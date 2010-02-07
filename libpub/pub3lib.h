// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3ast.h"
#include "pub3eval.h"

namespace pub3 {

  const char *okws_version_str ();
  u_int64_t okws_version_int ();

  //-----------------------------------------------------------------------

  class compiled_fn_t : public expr_t, public callable_t {
  public:
    compiled_fn_t (str lib, str n);
    virtual ~compiled_fn_t () {}

    virtual ptr<const expr_t> eval_to_val (publish_t *e, args_t args) const;
    virtual void pub_to_val (publish_t *p, args_t args, cxev_t, CLOSURE) const;
    str to_str (bool q = false) const;
    bool to_xdr (xpub3_expr_t *x) const { return false; }
    virtual str name () const { return _name; }
    void set_name (str lib, str n);

  protected:
    virtual bool safe_args () const { return true; }

    // evaluate, given that the arguments have been prevaluted...
    virtual ptr<const expr_t> 
    v_eval_1 (publish_t *e, const cargs_t &args) const { return NULL; }

    ptr<const callable_t> to_callable () const { return mkref (this); }
    const char *get_obj_name () const { return "rfn1::runtime_fn_t"; }

    void pub_args (publish_t *p, args_t in, cargs_t *out, evv_t ev, CLOSURE) 
      const;
    void eval_args (publish_t *p, args_t in, cargs_t *out) const;

    str _lib, _name;
  };

  //-----------------------------------------------------------------------

  class compiled_handrolled_fn_t : public compiled_fn_t {
  public:
    compiled_handrolled_fn_t (str lib, str n) : compiled_fn_t (lib, n) {}
    bool might_block () const { return true; }
    ptr<const expr_t> eval_to_val (publish_t *e, args_t args) 
      const { return NULL; }
  };

  //-----------------------------------------------------------------------

  class patterned_fn_t : public compiled_fn_t {
  public:
    patterned_fn_t (str l, str n, str p) : compiled_fn_t (l, n), _arg_pat (p) {}
    virtual bool might_block () const { return false; }
    
  protected:

    struct arg_t {
      arg_t () : _i (0), _u (0), _b (-1), _n (-1), _f (0) {}
      ptr<const expr_t> _O;
      ptr<rxx> _r;
      str _s;
      ptr<const expr_dict_t> _d;
      ptr<const expr_list_t> _l;
      int64_t _i;
      u_int64_t _u;
      short _b;
      short _n;
      double _f;
    };

    // evaluate, given that the arguments have been prevaluted...
    ptr<const expr_t> v_eval_1 (publish_t *e, const cargs_t &args) const;

    // evaluate, given that the args have been preevaluated and type-checked
    virtual ptr<const expr_t> 
    v_eval_2 (publish_t *e, const vec<arg_t> &args) const = 0;

    virtual bool check_args (publish_t *p, const cargs_t &args, 
			     vec<arg_t> *a) const;

    str _arg_pat;
  };

  //-----------------------------------------------------------------------

  class compiled_fn_t;

  //
  //  A library of runtime functions.  These functions can be
  //  bound into a specified environment.
  //
  class library_t {
  public:
    library_t () {}
    ~library_t () {}
    void bind (ptr<bindtab_t> b);
    ptr<bindtab_t> bind ();
    static void import (ptr<library_t> l);
    static void clear_all ();
  protected:
    vec<ptr<compiled_fn_t> > _functions;
  };

  //-----------------------------------------------------------------------

  //
  // use the macros to define a lot of library methods at the same time,
  // without much effort. Should be used in a namespace that's different
  // from pub3.  The nasespace ought to have:
  //
  //   extern conat char *libname;
  //
  // so that the following macros might work.  See librfn/okrfn.h for
  // an example.
  //
  
#define PUB3_COMPILED_FN(x,pat)				                \
  class x##_t : public pub3::patterned_fn_t {				\
  public:								\
    x##_t () : patterned_fn_t (libname, #x, pat) {}			\
    ptr<const expr_t>							\
    v_eval_2 (publish_t *p, const vec<arg_t> &args) const;		\
  }
  
#define PUB3_FILTER(x)					     \
  class x##_t : public pub3::patterned_fn_t {		     \
  public:						     \
  x##_t () : patterned_fn_t (libname, #x, "s") {}	     \
  static str filter (str s);				     \
  ptr<const expr_t>					     \
  v_eval_2 (publish_t *p, const vec<arg_t> &args) const	     \
    { return expr_str_t::safe_alloc (filter (args[0]._s)); } \
  }

#define PUB3_COMPILED_HANDROLLED_FN(x)					\
  class x##_t : public pub3::compiled_handrolled_fn_t {			\
  public:								\
    x##_t () : compiled_handrolled_fn_t (libname, #x) {}		\
    void pub_to_val (publish_t *p, args_t args, cxev_t, CLOSURE) const;	\
  }

  //-----------------------------------------------------------------------

};
