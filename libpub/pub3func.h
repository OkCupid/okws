// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3ast.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  struct proc_core_t {
    proc_core_t (str nm) : _name (nm) {}
    static ptr<proc_core_t> alloc (str nm) 
    { return New refcounted<proc_core_t> (nm); }
    void add_params (ptr<identifier_list_t> l);
    str _name;
    ptr<identifier_list_t> _params;
    ptr<zone_t> _body;
  };

  //-----------------------------------------------------------------------

  class proc_call_t : public expr_t {
  public:
    proc_call_t (ptr<proc_core_t> c, const location_t &l) 
      : _core (c), _location (l) {}
    str to_str (bool q = false) const;
    str to_str_short () const;

    void pub_to_val (publish_t e, ptr<const expr_list_t> l, cxev_t ev, CLOSURE) 
      const;
    void pub_to_ref (publish_t e, ptr<const expr_list_t> l, mrev_t ev, CLOSURE)
      const;

    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    const ptr<const proc_core_t> _core;
    const location_t _location;
  };

  //-----------------------------------------------------------------------

  class proc_def_t : public statement_t {
  public:
    proc_def_t (str nm, location_t l) 
      : statement_t (l),
	_core (proc_core_t::alloc (nm)) {}
    proc_def_t (const xpub3_proc_def_t &x) ;
    static ptr<proc_def_t> alloc (str nm);
    void add_params (ptr<identifier_list_t> p);
    void add_body (ptr<zone_t> z);
    
    bool publish_nonblock (publish_t p) const;
    bool might_block () const { return false; }
    ptr<const proc_core_t> core () const { return _core; }
    ptr<proc_call_t> alloc_call () const;
  protected:
    ptr<proc_core_t> _core;
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

