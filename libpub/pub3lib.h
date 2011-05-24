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

    virtual ptr<const expr_t> eval_to_val (eval_t *e, args_t args) const;
    virtual void pub_to_val (eval_t *p, args_t args, cxev_t, CLOSURE) const;
    str to_str (PUB3_TO_STR_ARG) const;
    bool to_xdr (xpub3_expr_t *x) const { return false; }
    virtual str name () const { return _name; }
    void set_name (str lib, str n);

  protected:
    virtual bool safe_args () const { return true; }

    // evaluate, given that the arguments have been prevaluted...
    virtual ptr<const expr_t> 
    v_eval_1 (eval_t *e, const margs_t &args) const { return NULL; }

    ptr<const callable_t> to_callable () const { return mkref (this); }
    const char *get_obj_name () const { return "rfn1::runtime_fn_t"; }

    void pub_args (eval_t *p, args_t in, margs_t *out, evv_t ev, CLOSURE) 
      const;
    void eval_args (eval_t *e, args_t in, margs_t *out) const;

    str _lib, _name;
  };

  //-----------------------------------------------------------------------

  class patterned_fn_base_t : public compiled_fn_t {
  public:
    patterned_fn_base_t (str l, str n, str p) 
      : compiled_fn_t (l, n), _arg_pat (p) {}

    struct arg_t {
      arg_t () : _i (0), _u (0), _b (false), _n (-1), _f (0) {}
      ptr<expr_t> _O;
      ptr<rxx> _r;
      str _s;
      ptr<expr_dict_t> _d;
      ptr<expr_list_t> _l;
      int64_t _i;
      u_int64_t _u;
      bool _b;
      short _n;
      double _f;
      ptr<const callable_t> _F;
    };

  protected:

    virtual bool check_args (eval_t *p, const margs_t &args, 
			     vec<arg_t> *a) const;

    str _arg_pat;
  };

  //-----------------------------------------------------------------------

  // Only works for non-blocking function bodies
  class patterned_fn_t : public patterned_fn_base_t {
  public:
    patterned_fn_t (str l, str n, str p) 
      : patterned_fn_base_t (l, n, p) {}
    
    virtual bool might_block () const { return false; }
    // evaluate, given that the arguments have been prevaluted...
    ptr<const expr_t> v_eval_1 (eval_t *e, const margs_t &args) const;
    
    // evaluate, given that the args have been preevaluated and type-checked
    virtual ptr<const expr_t> 
    v_eval_2 (eval_t *e, const vec<arg_t> &args) const = 0;
  };

  //-----------------------------------------------------------------------

  class patterned_fn_blocking_t : public patterned_fn_base_t {
  public:
    patterned_fn_blocking_t (str l, str n, str p) 
      : patterned_fn_base_t (l, n, p) {}
    
    bool might_block () const { return true; }
    
    // pub, given that the args have been preevaluated and type-checked
    virtual void
    v_pub_to_val_2 (eval_t *, const vec<arg_t> &, cxev_t, CLOSURE) const = 0;

    void pub_to_val (eval_t *p, args_t args, cxev_t, CLOSURE) const;
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
    void bind_all (str s);
  protected:
    vec<ptr<compiled_fn_t> > _functions;
    str _all_libname;
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
    v_eval_2 (eval_t *p, const vec<arg_t> &args) const;			\
  }
  
#define PUB3_FILTER(x)					     \
  class x##_t : public pub3::patterned_fn_t {		     \
  public:						     \
  x##_t () : patterned_fn_t (libname, #x, "s") {}	     \
  static str filter (str s);				     \
  ptr<const expr_t>					     \
  v_eval_2 (eval_t *e, const vec<arg_t> &args) const	     \
    { return expr_str_t::safe_alloc (filter (args[0]._s)); } \
  }

#define PUB3_COMPILED_HANDROLLED_FN(x)					\
  class x##_t : public pub3::compiled_fn_t {				\
  public:								\
  x##_t () : compiled_fn_t (libname, #x) {}				\
  ptr<const expr_t> eval_to_val (eval_t *e, args_t args) const;		\
  void pub_to_val (eval_t *p, args_t args, cxev_t, CLOSURE) const;	\
  bool count_args (eval_t *p, size_t s) const;			\
  }
  
#define PUB3_COMPILED_UNPATTERNED_FN(x)					\
  class x##_t : public pub3::compiled_fn_t {				\
  public:								\
  x##_t () : compiled_fn_t (libname, #x) {}				\
  ptr<const expr_t>							\
  v_eval_1 (eval_t *p, const margs_t &args) const;			\
  }
  
#define PUB3_COMPILED_FN_BLOCKING(x,pat)			        \
  class x##_t : public pub3::patterned_fn_blocking_t {			\
  public:								\
  x##_t () : patterned_fn_blocking_t (libname, #x, pat) {}		\
  void									\
  v_pub_to_val_2 (eval_t *, const vec<arg_t> &, cxev_t, CLOSURE) const; \
  }
  
  //-----------------------------------------------------------------------

};
