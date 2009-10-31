// -*-c++-*-
#include "pub3func.h"
#include "pub3parse.h"


//-----------------------------------------------------------------------
//
// Functions for allocations runtime functions (rfn's)

namespace pub3 {

  //-----------------------------------------------------------------------

  ptr<rfn_factory_t> rfn_factory_t::_curr;

  //-----------------------------------------------------------------------
  
  void
  rfn_factory_t::set (ptr<rfn_factory_t> f)
  {
    _curr = f;
  }
  
  //-----------------------------------------------------------------------
  
  ptr<rfn_factory_t>
  rfn_factory_t::get ()
  {
    if (!_curr) {
      _curr = New refcounted<null_rfn_factory_t> ();
    }
    return _curr;
  }
    
  //-----------------------------------------------------------------------

  ptr<const expr_t>
  error_fn_t::eval_to_val (eval_t e) const
  {
    e.report_error (_err, _lineno);
    return NULL;
  }

  //-----------------------------------------------------------------------

  ptr<runtime_fn_t> 
  null_rfn_factory_t::alloc (const str &s, ptr<expr_list_t> l, int lineno)
  {
    strbuf b ("null library; function '%s' does not exist here", s.cstr ());
    return New refcounted<error_fn_t> (s, l, lineno, b);
  }

  //-----------------------------------------------------------------------

  ptr<runtime_fn_stub_t> 
  runtime_fn_stub_t::alloc (const str &n, ptr<expr_list_t> l)
  { return New refcounted<runtime_fn_stub_t> (n, l, plineno ()); }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  runtime_fn_stub_t::get_rfn () const
  {
    if (!_rfn) {
      _rfn = rfn_factory_t::get ()->alloc (_name, _arglist, _lineno);
    }
    return _rfn;
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t> 
  runtime_fn_stub_t::eval_to_val (eval_t e) const
  {
    return get_rfn ()->eval_to_val (e);
  }

  //-----------------------------------------------------------------------
  
  bool 
  runtime_fn_stub_t::unshift_argument (ptr<expr_t> e)
  {
    if (!_arglist) {
      _arglist = New refcounted<expr_list_t> ();
    }
    _arglist->push_front (e);
    return true;
  }

  //-----------------------------------------------------------------------

  void
  proc_def_t::add_params (ptr<identifier_list_t> l)
  {
    _core->add_params (l);
  }

  //-----------------------------------------------------------------------

  void 
  proc_core_t::add_params (ptr<identifier_list_t> l)
  {
    if (!_params) {
      _params = l;
    } else if (l) { 
      *_params += *l;
    }
  }

  //-----------------------------------------------------------------------

  void
  proc_def_t::add_body (ptr<zone_t> z) 
  {
    _core->_body = z;
  }

  //-----------------------------------------------------------------------

  bool
  proc_def_t::publish_nonblock (publish_t p) const
  {
    p.env ()->add_global_binding (_core->_name, alloc_call ());
    return true;
  }

  //-----------------------------------------------------------------------

  ptr<proc_call_t> proc_def_t::alloc_call () const
  { return New refcounted<proc_call_t> (_core, _location); }

  //-----------------------------------------------------------------------

  str 
  proc_call_t::to_str_short () const
  {
    str ls = _location.to_str ();
    return strbuf ("<%s @ %s>", _core->_name.cstr (), ls.cstr ());
  }


  //-----------------------------------------------------------------------

  str
  proc_call_t::to_str (bool q) const
  {
    str ss = to_str_short ();
    return strbuf ("pub-defined runtime procedure %s", ss.cstr ());
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  proc_call_t::eval_to_val (eval_t e, ptr<const expr_list_t> l) const
  {
    size_t gs = l->size ();
    size_t es = _core->_params->size ();
    str s = to_str_short ();
    ptr<const expr_t> ret;
    if (gs != es) {
      strbuf b ("Wrong number of parameters to function call %s "
		"(expected %zd, got %zd)", s.cstr (), es, gs);
      report_error (e, b);
    } else {
      // Do some eval here; for now, don't do anything because I haven't
      // figured out what to do about calling 

    }
    return ret;
  }

  //-----------------------------------------------------------------------

};

//
// end RFNs
//-----------------------------------------------------------------------
