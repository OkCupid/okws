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
  fndef_t::add_params (ptr<identifier_list_t> l)
  {
    if (!_params) {
      _params = l;
    } else if (l) { 
      *_params += *l;
    }
  }

  //-----------------------------------------------------------------------

  void
  fndef_t::add_body (ptr<zone_t> z) 
  {
    _body = z;
  }

  //-----------------------------------------------------------------------

  bool
  fndef_t::publish_nonblock (publish_t p) const
  {
    p.env ()->add_global_binding (_name, expr_cow_t::alloc (mkref (this)));
    return true;
  }

  //-----------------------------------------------------------------------

  str
  fndef_t::to_str (bool q) const
  {
    return strbuf ("pub-defined runtime function <%s>", _name.cstr ());
  }

  //-----------------------------------------------------------------------

};

//
// end RFNs
//-----------------------------------------------------------------------
