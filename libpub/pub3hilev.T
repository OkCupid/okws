// -*-c++-*-

#include "pub3hilev.h"

namespace pub3 {

  typedef abstract_publisher_t ap_t;

  //=======================================================================

  global_t::global_t () : _universals (New refcounted<bindtab_t> ()) {}
  
  //-----------------------------------------------------------------------

  ptr<global_t> 
  global_t::get ()
  {
    static ptr<global_t> g;
    if (!g) { g = New refcounted<global_t> (); }
    return g;
  }

  //=======================================================================
  
  tamed void 
  ap_t::run (zbuf *b, str fn, evb_t ev, ptr<expr_dict_t> d, opts_t opts, 
	     status_t *sp, ptr<file_t> *fp)
  {
    tvars {
      publish_t pub (global_t::get ()->universals (), b);
      status_t status;
      ptr<file_t> file;
    }
    if (opts < 0) opts = _opts;

    pub.set_opts (opts);

    twait { publish_full (pub, fn, 0, mkevent (status, file)); }

    if (sp) *sp = status;
    if (fp) *fp = file;
    ev->trigger (status.status == XPUB_STATUS_OK);
  }

  //-----------------------------------------------------------------------


};
