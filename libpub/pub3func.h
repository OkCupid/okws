// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3ast.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class callable_t {
  public:
    typedef ptr<const expr_list_t> args_t;
    callable_t () {}
    virtual ~callable_t () {}
    
    virtual bool might_block () const { return false; }

    virtual ptr<const expr_t> eval_to_val (publish_t p, args_t args) const = 0;
    virtual void pub_to_val (publish_t p, args_t args, cxev_t, CLOSURE) const;
  };

  //-----------------------------------------------------------------------

  class lambda_t : public expr_t, public callable_t {
  public:
    lambda_t (ptr<identifier_list_t> p, ptr<zone_t> z, location_t loc);
    lambda_t (const xpub3_lambda_t &l);
    bool to_xdr (xpub3_lambda_t *x) const;
    bool to_xdr (xpub3_expr_t *x) const;
    static ptr<lambda_t> alloc (ptr<identifier_list_t> p, ptr<zone_t> z);
    static ptr<lambda_t> alloc (const xpub3_lambda_t &l);
    void set_name (str n) { _name = n; }
    str to_str () const;
    bool might_block () const;

    ptr<const expr_t> eval_to_val (publish_t p, args_t args) const;
    ptr<const expr_t> eval_to_val (eval_t e) const;
    ptr<mref_t> eval_to_ref (eval_t e) const;
    void pub_to_val (publish_t p, args_t args, cxev_t, CLOSURE) const;
    ptr<const callable_t> to_callable () const { return mkref (this); }
    ptr<lambda_t> copy (eval_t e) const;
  protected:
    bool check_args (publish_t p, args_t a) const;
    ptr<bindtab_t> bind_args_nonblock (publish_t p, args_t a) const;
    void bind_arg (ptr<bindtab_t> t, size_t i, ptr<mref_t> r) const;
    void bind_args (publish_t p, args_t a, event<ptr<bindtab_t> >::ref ev,
		    CLOSURE) const;
    bool is_static () const { return false; }

    location_t _loc;
    str _name;
    ptr<identifier_list_t> _params;
    ptr<zone_t> _body;

    // Evaluated lambdas have the metadata of where they came from
    // all populated.  Those that haven't been evaluated do not.
    ptr<const metadata_t> _md;
  };

  //-----------------------------------------------------------------------

  class fndef_t : public statement_t  {
  public:
    fndef_t (str nm, ptr<lambda_t> d, location_t l) 
      : statement_t (l), _name (nm), _lambda (d) { d->set_name (nm); }
    fndef_t (const xpub3_fndef_t &x) ;
    static ptr<fndef_t> alloc (const xpub3_fndef_t &x);
    static ptr<fndef_t> alloc (str nm, ptr<lambda_t> d);
    status_t v_publish_nonblock (publish_t p) const;
    bool might_block_uncached () const { return false; }
    bool to_xdr (xpub3_statement_t *x) const;
  protected:
    str _name;
    ptr<lambda_t> _lambda;
  };

  //-----------------------------------------------------------------------

  class call_t : public expr_t {
  public:
    call_t (ptr<expr_t> x, ptr<expr_list_t> a, lineno_t l) 
      : expr_t (l), _fn (x), _arglist (a) {}
    call_t (const xpub3_call_t &call);
    
    ptr<expr_list_t> args () const { return _arglist; }

    static ptr<call_t> alloc (const xpub3_call_t &call);
    static ptr<call_t> alloc (ptr<expr_t> x, ptr<expr_list_t> l);

    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::call_t"; }

    ptr<const expr_t> eval_to_val (eval_t e) const { return NULL; }
    ptr<mref_t> eval_to_ref (eval_t e) const { return NULL; }
    bool might_block_uncached () const { return true; }

    void pub_to_val (publish_t p, cxev_t ev, CLOSURE) const;
    void pub_to_ref (publish_t p, mrev_t ev, CLOSURE) const;

    void unshift_argument (ptr<expr_t> x);
    ptr<call_t> coerce_to_call () { return mkref (this); }

  protected:
    void pub_prepare (publish_t p, event<ptr<const callable_t> >::ref ev, 
		      CLOSURE) const;
    ptr<expr_t> _fn;
    ptr<expr_list_t> _arglist;
  };

  //-----------------------------------------------------------------------

  class fn_call_t : public expr_t {
  public:
    fn_call_t (str &n, ptr<expr_list_t> a, lineno_t l);
    bool to_xdr (xpub3_expr_t *x) const { return false; }
    static ptr<fn_call_t> alloc (const str &n, ptr<expr_list_t> a, lineno_t l);

  protected:
    ptr<const expr_list_t> _arglist;
  };

  //-----------------------------------------------------------------------

  // temp
  class runtime_fn_t;

  //
  // rfn_factory: runtime function factory
  //
  //  allocates runtime functions.  OKWS core supports some of them
  //  but specific okws instances are welcome to support more.  
  //  details still uncertain, but it will most likely involve
  //  local changes to pubd (via dynamic loading?) and also changes
  //  to specific services -- similar to the Apache module system...
  //
  class rfn_factory_t {
  public:
    rfn_factory_t () {}
    virtual ~rfn_factory_t () {}

    virtual ptr<runtime_fn_t>
    alloc (const str &s, ptr<expr_list_t> l, int lineno) = 0;

    // Access the singleton runtime function factory; by default
    // it's set to a null factory, but can be explanded any which
    // way.
    static ptr<rfn_factory_t> _curr;
    static void set (ptr<rfn_factory_t> f);
    static ptr<rfn_factory_t> get ();
  };

  //-----------------------------------------------------------------------

  class null_rfn_factory_t : public rfn_factory_t {
  public:
    null_rfn_factory_t  () : rfn_factory_t () {}
    ptr<runtime_fn_t> alloc (const str &s, ptr<expr_list_t> l, int lineno);

  };

  //-----------------------------------------------------------------------
};

