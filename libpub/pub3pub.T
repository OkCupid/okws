// -*-c++-*-
#include "pub3pub.h"
#include "pub3file.h"
#include "pub3ast.h"
#include "pub3hilev.h"

namespace pub3 {

  //====================================================================

  tamed void
  publish_t::publish (str nm, location_t loc, ptr<bind_interface_t> d, 
		      status_ev_t ev)
  {
    tvars {
      size_t sz;
      xpub_status_t status;
      ptr<file_t> dummy;
    }
    push_include_location (loc);
    sz = env ()->push_locals (d);
    twait { _pub_iface->publish (*this, nm, mkevent (status, dummy)); }
    env ()->pop_to (sz);
    pop_include_location ();
    ev->trigger (status);
  }

  //--------------------------------------------------------------------

  ptr<localizer_t>
  publish_t::localizer () 
  {
    ptr<localizer_t> ret;
    if (!(_opts & int (P_NOLOCALE))) {
      ret = _localizer;
    }
    return ret;
  }

  //--------------------------------------------------------------------

  publish_t::publish_t (ptr<bindtab_t> universals, zbuf *z)
    : eval_t (New refcounted<env_t> (universals), output_t::alloc (z)),
      _opts (0) {}

  //--------------------------------------------------------------------

  tamed void
  publish_t::publish_file (ptr<const file_t> file, status_ev_t ev)
  {
    tvars {
      ptr<const metadata_t> md;
      xpub_status_t status;
      size_t sz;
    }

    if (_include_stack.size () > ok_pub_max_include_depth) {
      str fn = file->metadata ()->jailed_filename ();
      strbuf msg;
      msg << "maximum include stack depth (" << ok_pub_max_include_depth 
	  << ") exceeded when trying to include file " << fn;
      output_err_stacktrace (msg);
    } else {
      md = file->metadata ();
      push_metadata (md);
      sz = env ()->push_locals (md->to_dict ());
      twait { file->data ()->publish (*this, mkevent (status)); }
      env ()->pop_to (sz);
      pop_metadata ();
    }
    ev->trigger (status);
  }

  //--------------------------------------------------------------------

  void publish_t::push_include_location (location_t l) 
  { _include_stack.push_back (l); }
  void publish_t::push_metadata (ptr<const metadata_t> md)
  { _metadata_stack.push_back (md); }
  void publish_t::pop_include_location () { _include_stack.pop_back (); }
  void publish_t::pop_metadata () { _metadata_stack.pop_back (); }

  //--------------------------------------------------------------------

  void
  publish_t::output_err_stacktrace (str msg)
  {


  }

  //--------------------------------------------------------------------

  str
  publish_t::set_cwd (str s)
  {
    str ret = _cwd;
    _cwd = s;
    return ret;
  }

  //--------------------------------------------------------------------


  

  //====================================================================

  ptr<output_t>
  output_t::alloc (zbuf *z)
  {
    ptr<output_t> ret;
    if (z) { ret = New refcounted<output_std_t> (z);   }
    else   { ret = New refcounted<output_silent_t> (); }
    return ret;
  }

  //====================================================================

};


