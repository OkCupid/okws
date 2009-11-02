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

  //====================================================================

  publish_t::publish_t (ptr<bindtab_t> universals, zbuf *z)
    : eval_t (New refcounted<env_t> (universals), output_t::alloc (z)),
      _opts (0) {}

  //====================================================================

};


