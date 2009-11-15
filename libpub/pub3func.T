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

  ptr<call_t> call_t::alloc (const str &n, ptr<expr_list_t> l)
  { return New refcounted<call_t> (n, l, plineno ()); }

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
    p.env ()->add_global_binding (_core->_name, mkref (this)); }
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

  bool
  proc_call_t::might_block_uncached () const
  {
    ptr<const zone_t> b;
    return (b = _core->body ()) && b->might_block ();
  }

  //-----------------------------------------------------------------------

  str
  proc_call_t::to_str (bool q) const
  {
    str ss = to_str_short ();
    return strbuf ("pub-defined runtime procedure %s", ss.cstr ());
  }

  //-----------------------------------------------------------------------

  bool
  proc_call_t::check_args (publish_t e) const
  {
    size_t gs, es;
    ptr<const expr_list_t> l = _args;
    gs = l->size ();
    es = _core->_params->size ();
    str s = to_str_short ();
    bool ret = true;

    if (gs != es) {
      strbuf b ("Wrong number of parameters to function call %s "
		"(expected %zd, got %zd)", s.cstr (), es, gs);
      report_error (e, b);
      ret = false;
    } 
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void
  proc_call_t::pub_to_val (publish_t e, cxev_t ev) const
  {
    tvars {
      ptr<const expr_t> ret;
      ptr<control_t> old_ctrl;
      ptr<const expr_list_t> l;
    }

    l = _args;

    if (check_args (e, l)) {
      old_ctrl = e.push_control ();
      ret = e.control ()->rtrn ();
      e.restore_control (old_ctrl);
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

};

//
// end RFNs
//-----------------------------------------------------------------------
