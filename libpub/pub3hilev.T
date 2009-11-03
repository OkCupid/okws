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
    pub.set_pub_iface (mkref (this));

    twait { publish (pub, fn, mkevent (status, file)); }

    if (sp) *sp = status;
    if (fp) *fp = file;

    pub.set_pub_iface (NULL);

    ev->trigger (status.status == XPUB_STATUS_OK);
  }

  //-----------------------------------------------------------------------
  
  tamed void 
  ap_t::publish (publish_t pub, str fn, getfile_ev_t ev)
  {
    tvars {
      status_t status (XPUB_STATUS_NOENT);
      ptr<file_t> file;
      str err;
      vec<str> files_to_check;
      size_t i;
      str d, f, cwd_prev;
      ptr<const localizer_t> localizer;
    }

    // generate a list of internationalized files.  if that list is
    // empty, we'll skip the following loop and we'll just
    // fall through with status.status == XPUB_STATUS_NOENT
    localizer = pub.localizer ();
    list_files_to_check (pub.cwd (), fn, &files_to_check, localizer);

    for (i = 0; i < files_to_check.size (); i++) {

      twait { 
	getfile (files_to_check[i], mkevent (status, file), pub.opts ()); 
      }
      if (status.status != XPUB_STATUS_NOENT) {
	break;
      }
    }

    switch (status.status) {
    case XPUB_STATUS_OK:

      assert (file);
      assert (i < files_to_check.size ());
      basename_dirname (files_to_check[i], &d, &f);

      cwd_prev = pub.set_cwd (d);

      twait { pub.publish_file (file, mkevent (status)); }
      if (status.status != XPUB_STATUS_OK) 
	err = strbuf () << "error in publishing file: " << fn;

      // Reset the working directory to whatever it was previously.
      pub.set_cwd (cwd_prev);
      break;

    case XPUB_STATUS_RPC_ERR:
      err = strbuf () << fn << ": RPC error: " << *status.rpc_err;
      break;
    case XPUB_STATUS_NOENT:
      err = strbuf () << fn << ": file not found";
      break;
    default:
      err = *status.error;
      break;
    }
    
    if (err) {
      pub.output_err (err);
    }
    ev->trigger (status, file);
  }
  
  //-----------------------------------------------------------------------
  
  static void use_file (vec<str> *out, str f) { if (f) out->push_back (f); }
  
  //-----------------------------------------------------------------------
  
  void
  ap_t::list_files_to_check (str cwd, str nm, vec<str> *out, 
			     ptr<const localizer_t> l)
  {
    str d,f;
    bool abs = basename_dirname (nm, &d, &f);
    if (!f) return;
    bool use_defs = !(l && l->strict ()) ;
    
    if (abs) {
      if (l)        use_file (out, l->localize (nm)); 
      if (use_defs) use_file (out, nm);
    } else {
      
      str n1, n2;
      strbuf b;
      if (is_remote ()) b << "/";
      b << nm;
      n1 = b;
      
      if (cwd) {
	n2 = apply_container_dir (cwd, nm);
      }

      if (l) {
	if (n1) use_file (out, l->localize (n1));
	if (n2) use_file (out, l->localize (n2));
      }
 
      if (use_defs) {
	if (n1) use_file (out, n1);
	if (n2) use_file (out, n2);
      }
    }

    if (l) { use_file (out, l->get_default_fnf ()); }
  }

  //---------------------------------------------------------------------


};
