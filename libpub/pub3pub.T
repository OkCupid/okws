// -*-c++-*-
#include "pub3pub.h"

namespace pub3 {

  //====================================================================

  tamed void
  publish_t::publish (str nm, ptr<bind_interface_t> d, status_ev_t ev)
  {
    tvars {
      size_t sz;
      xpub_status_t status;
    }
    sz = env ()->push_locals (d);
    twait { publish2 (nm, mkevent (status)); }
    env ()->pop_to (sz);
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

  str
  publish_t::set_cwd (str s)
  {
    str ret = _cwd;
    _cwd = s;
    return ret;
  }

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


