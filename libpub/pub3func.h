// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3ast.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class fndef_t : public statement_t, public expr_t {
  public:
    fndef_t (str nm, location_t l) : statement_t (l), _name (nm) {}
    fndef_t (const xpub3_fndef_t &x) ;
    static ptr<fndef_t> alloc (str nm);
    void add_params (ptr<identifier_list_t> p);
    void add_body (ptr<zone_t> z);

    str to_str (bool q = false) const;
    bool publish_nonblock (publish_t p) const;
    bool might_block () const { return false; }
    ptr<const fndef_t> to_fndef () const { return mkref (this); }
    ptr<fndef_t> to_fndef () { return mkref (this); }
  protected:
    str _name;
    ptr<identifier_list_t> _params;
    ptr<zone_t> _body;
  };

  //-----------------------------------------------------------------------

  class runtime_fn_t : public expr_t {
  public:
    runtime_fn_t (const str &n, ptr<expr_list_t> a, int l) 
      : expr_t (l), _name (n), _arglist (a) {}

    ptr<expr_list_t> args () const { return _arglist; }
    str name () const { return _name; }

    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::runtime_fn_t"; }

    ptr<const expr_t> eval_to_val (eval_t e) const { return NULL; }

  protected:
    str _name;
    ptr<expr_list_t> _arglist;
  };

  //-----------------------------------------------------------------------

  class runtime_fn_stub_t : public runtime_fn_t {
  public:
    runtime_fn_stub_t (const str &n, ptr<expr_list_t> a, int l) 
      : runtime_fn_t (n, a, l) {}

    static ptr<runtime_fn_stub_t> alloc (const str &n, ptr<expr_list_t> l);
    const char *get_obj_name () const { return "pub3::runtime_fn_stub_t"; }
    ptr<const expr_t> eval_to_val (eval_t e) const;
    bool unshift_argument (ptr<expr_t> e);

  protected:
    mutable ptr<const expr_t> _rfn;
    ptr<const expr_t> get_rfn () const;
  };

  //-----------------------------------------------------------------------

  class error_fn_t : public runtime_fn_t {
  public:
    error_fn_t (const str &n, ptr<expr_list_t> a, int l, const str &err)
      : runtime_fn_t (n, a, l), _err (err) {}
    ptr<const expr_t> eval_to_val (eval_t e) const;
    const char *get_obj_name () const { return "pub3::error_fn_t"; }
  protected:
    str _err;
  };

  //-----------------------------------------------------------------------

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

    ptr<runtime_fn_t> alloc (const xpub3_fn_t &x);

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

