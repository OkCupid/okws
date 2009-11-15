// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */

#pragma once

#include "pub.h"
#include "pub3ast.h"
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  // Procedures are defined in pub/html code via the 'def' keyword.
  struct proc_core_t {
    proc_core_t (str nm) : _name (nm) {}
    static ptr<proc_core_t> alloc (str nm) 
    { return New refcounted<proc_core_t> (nm); }
    void add_params (ptr<identifier_list_t> l);
    ptr<zone_t> body () { return _body; }
    ptr<const zone_t> body () const { return _body; }
    str _name;
    ptr<identifier_list_t> _params;
    ptr<zone_t> _body;
  };

  //-----------------------------------------------------------------------

  class proc_call_t : public expr_t {
  public:
    proc_call_t (ptr<proc_core_t> c, ptr<const expr_list_t> args, 
		 const location_t &l) 
      : _core (c), _args (args), _location (l) {}
    str to_str (bool q = false) const;
    str to_str_short () const;
    
    void pub_to_val (publish_t e, cxev_t ev, CLOSURE) const;
    void pub_to_ref (publish_t e, mrev_t ev, CLOSURE) const;
    bool might_block_uncached () const;

    ptr<proc_call_t> to_proc_call () { return mkref (this); }
    ptr<const proc_call_t> to_proc_call () const { return mkref (this); }

    bool to_xdr (xpub3_expr_t *x) const;
  protected:
    bool check_args (publish_t e, ptr<const expr_list_t> l) const;
    const ptr<const proc_core_t> _core;
    const ptr<const expr_list_t> _args;
    const lineno_t _location;
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
    ptr<proc_call_t> alloc_call (ptr<const expr_list_t> a, lineno_t l) const;
  protected:
    ptr<proc_core_t> _core;
  };

  //-----------------------------------------------------------------------

  class call_t : public expr_t {
  public:
    call_t (const str &n, ptr<expr_list_t> a, int l) 
      : expr_t (l), _name (n), _arglist (a) {}
    call_t (const xpub3_call_t &call);
    
    ptr<expr_list_t> args () const { return _arglist; }
    str name () const { return _name; }

    static ptr<call_t> alloc (const xpub3_call_t &call);
    static ptr<call_t> alloc (const str &nm, ptr<expr_list_t> l);

    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::call_t"; }

    ptr<const expr_t> eval_to_val (eval_t e) const;
    ptr<mref_t> eval_to_ref (eval_t e) const;

    void pub_to_val (publish_t p, cxev_t ev, CLOSURE) const;
    void pub_to_ref (publish_t p, mrev_t ev, CLOSURE) const;

  protected:
    str _name;
    ptr<expr_list_t> _arglist;
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

