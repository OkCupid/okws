// -*-c++-*-
/* $Id$ */

#include "pub2.h"
#include "okdbg.h"

namespace pub2 {
  
  file_lookup_t g_file_nocache_lookup;

  typedef abstract_publisher_t ap_t;
  typedef remote_publisher_t rp_t;
  typedef caching_remote_publisher_t crp_t;
  typedef output_conf2_t oc_t;
  typedef local_publisher_t lp_t;

  TAMED void 
  ap_t::publish_T (output_t *o, pfnm_t fn, penv_t *env, int lineno,
		   status_cb_t cb)
  {
    VARS {
      status_t status (XPUB_STATUS_OK);
      ptr<bound_pfile2_t> bpf;
      str err;
      vec<str> files_to_check;
      size_t i;
      str d,f, cwd_prev;
    }

    list_files_to_check (fn, &files_to_check);

    if (files_to_check.size () == 0) {
      status.set_status (XPUB_STATUS_NOENT);
      SIGNAL (cb, status);
      return;
    }

    for (i = 0; i < files_to_check.size (); i++) {
      BLOCK { getfile (files_to_check[i], @(status, bpf), o->opts ()); }
      if (status.status != XPUB_STATUS_NOENT)
	break;
    }

    switch (status.status) {
    case XPUB_STATUS_OK:

      assert (bpf);
      assert (i < files_to_check.size ());

      basename_dirname (files_to_check[i], &d, &f);

      cwd_prev = _cwd;
      _cwd = d;

      BLOCK { bpf->bpf ()->publish (this, o, env, @(status)); }
      if (status.status != XPUB_STATUS_OK) 
	err = strbuf () << "error in publishing file: " << fn;

      // Reset the working directory to whatever it was previously.
      _cwd = cwd_prev;

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
    
    if (err)
      o->output_err (env, err, lineno);
    
    SIGNAL (cb, status);
  }
  
  TAMED void 
  ap_t::run (zbuf *b, pfnm_t nm, cbb cb, aarr_t *a, 
	     u_int op, penv_t *e)
  {
    VARS { status_t status; }
    BLOCK { run_full (b, nm, @(status), a, op, e); }
    SIGNAL (cb, status.status == XPUB_STATUS_OK);
  }

  
  TAMED void 
  ap_t::run_full (zbuf *b, pfnm_t nm, status_cb_t cb,
		  aarr_t *a, u_int op, penv_t *env)
  {
    VARS {
      ptr<output_std_t> o;
      penv_state_t *st;
      status_t status;
      bool XXX;
      pfile_type_t typ;
    }
    if (op == 0)
      op = _opts;
    if (!env)
      env = &_genv;
    typ = (op & P_WSS) ? PFILE_TYPE_WH : PFILE_TYPE_H;
    o = New refcounted<output_std_t> (b, typ, op);
    st = env->start_output (a, op);
    BLOCK { publish (o, nm, env, 0, @(status)); }

    // XXX = ignore output from the following function
    XXX = env->finish_output (st);
    SIGNAL (cb, status);
  }

  TAMED void 
  rp_t::connect (cbb cb)
  {
    VARS {
      clnt_stat err;
      bool ret (true);
    }
    BLOCK { _cli->call (PUB2_NULL, NULL, NULL, @(err)); }
    if (err) {
      warn << "NULL call to pubd failed: " << err << "\n";
      ret = false;
    }
    SIGNAL (cb, ret);
  }

  TAMED void 
  crp_t::connect (cbb cb)
  {
    VARS { 
      clnt_stat stat;
      bool ret (true);
    }
    BLOCK { _cli->call (PUB2_GET_PUSHES, NULL, &_connected, @(stat)); }
    if (stat) {
      warn << "Intial RPC call to pubd-2 failed: " << stat << "\n";
      ret = false;
    } else if (!_connected) {
      warn << "pubd-2 denied request for file status pushes!\n";
    }
    SIGNAL (cb, ret);
  }

  void rp_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      lost_connection ();
      warn << "connection to pubd2 unexpectedly died!\n";
      
      // XXX- might make sense to do something less extreme.
      exit (0);
    } else {
      switch (sbp->proc ()) {
      case PUB2_PUSH_DELTAS:
	handle_new_deltas (sbp);
	break;
      default:
	sbp->reject (PROC_UNAVAIL);
	break;
      }
    }
  }

  void rp_t::handle_new_deltas (svccb *sbp)
  {
    sbp->replyref (false);
  }

  void crp_t::lost_connection ()
  {
    _connected = false;
    clear_cache ();
  }

  void crp_t::handle_new_deltas (svccb *sbp)
  {
    xpub2_delta_set_t *s = sbp->Xtmpl getarg<xpub2_delta_set_t> ();
    handle_new_deltas (*s);
    sbp->replyref (true);
  }

  void crp_t::rm_file (pfnm_t nm, const u_int &opt)
  {
    cache_key_t k (nm, opt);
    _getfile_cache.remove (k);
  }

  void crp_t::handle_new_deltas (const xpub2_delta_set_t &s)
  {
    if (_delta_id >= 0 && s.serial != _delta_id + 1) {
      okdbg_warn (ERROR, "Missed a delta update! Clearing cache!");
      _opts_map.clear ();
      _getfile_cache.clear ();
    } else {
      _delta_id = s.serial;
      for (u_int i = 0; i < s.files.size (); i++) {
	const str &n = s.files[i];
	ptr<bhash<u_int> > *h = _opts_map[n];
	if (h) {
	  (*h)->traverse (wrap (this, &crp_t::rm_file, n));
	  _opts_map.remove (n);
	  if (OKDBG2(PUB2_CACHE)) {
	    strbuf b;
	    b << "Removing file '" << n << "' from GETFILE cache";
	    okdbg_warn (CHATTER, b);
	  }
	}
      }
    }
  }

  TAMED void
  rp_t::getfile_body (pfnm_t nm, const xpub2_getfile_res_t *res, 
		      getfile_cb_t cb, u_int opts)
  {
    VARS {
      ptr<bound_pfile2_t> bpf;
      status_t status (XPUB_STATUS_OK);
      const xpub_file_t *file (NULL);
    }
    if (res->data->file.mode == XPUB_XFER_WHOLE) {
      file = res->data->file.whole;
    }
    if (file)
      bpf = New refcounted<bound_pfile2_t> (nm, res->data->stat, *file, opts);
    else {
      status.set_status (XPUB_STATUS_CORRUPTION);
      *status.error = "Chunked file corrupted in transfer";
    }
    (*cb) (status, bpf);
  }

  TAMED void 
  rp_t::getfile_T (pfnm_t nm, getfile_cb_t cb, u_int opts)
  {
    VARS {
      clnt_stat err;
      ptr<bound_pfile2_t> file;
      xpub2_getfile_arg_t arg;
      xpub2_getfile_res_t res;
      status_t status (XPUB_STATUS_OK);
      cache_key_t k (nm, opts);
    }
    if (!prepare_getfile (k, &arg, &file, &status)) {
      BLOCK { _cli->call (PUB2_GETFILE, &arg, &res, @(err)); }
      if (err) {
	warn << nm << ": error in RPC call to PUB2_GETFILE: " 
	     << err << "\n";
	status.set_status(XPUB_STATUS_RPC_ERR);
	*status.rpc_err = err;
      } else {
	switch (res.status) {
	case XPUB_STATUS_OK:
	  BLOCK { getfile_body (nm, &res, @(status, file), opts); }
	  if (status.status == XPUB_STATUS_OK) {
	    cache_getfile (k, file);
	  }
	  break;
	case XPUB_STATUS_NOCHANGE:
	  file = file_nochange (k);
	  assert (file);
	  break;
	case XPUB_STATUS_NOENT: 
	  {
	    cache_noent (nm);
	    strbuf b;
	    b << nm << ": file not found";
	    status.set_status (XPUB_STATUS_NOENT);
	    *status.error = b;
	  }
	  break;
	case XPUB_STATUS_ERR:
	  status.set_status (XPUB_STATUS_ERR);
	  *status.error = *res.error;
	  break;
	default:
	  warn << __FILE__ << ":" << __LOC__ 
	       << ": unexpected error condition\n";
	  status.set_status (res.status);
	  *status.error = strbuf () << nm << ": unspecified error";
	  break;
	}
      }
    }
    SIGNAL (cb, status, file);
  }
  
  bool crp_t::prepare_getfile (const cache_key_t &k, xpub2_getfile_arg_t *arg,
			       ptr<bound_pfile2_t> *f, status_t *status)
  {
    cached_getfile_t *c = _getfile_cache[k];
    str s = k.fn ();
    if (c) {
      *f = c->file ();
      return true;
    } else if (_noent_cache[s]) {
      status->set_status (XPUB_STATUS_NOENT);
      strbuf b;
      b << s << ": file not found (failure cached)";
      *status->error = b;
      *f = NULL;
      return true;
    } else 
      return rp_t::prepare_getfile (k, arg, f, status);
  }

  void crp_t::cache_noent (pfnm_t nm)
  {
    _noent_cache.insert (nm);

    // remove it from the positive cache
    ptr<bhash<u_int> > *h = _opts_map[nm];
    if (h)
      (*h)->traverse (wrap (this, &crp_t::rm_file, nm));
    _opts_map.remove (nm);
  }

  void crp_t::cache_getfile (const cache_key_t &k, ptr<bound_pfile2_t> f)
  {
    //
    // Make a mapping of filenames -> options, so that we know which
    // cache entries are associated with a given filename (in case it's
    // later removed from the cache.
    //
    ptr<bhash<u_int> > *hp, h;
    if (!( hp = _opts_map[k.fn ()] )) {
      h = New refcounted<bhash<u_int> > ();
      _opts_map.insert (k.fn (), h);
      hp = &h;
    }
    (*hp)->insert (k.opts ());

    _noent_cache.remove (k.fn ());

    _getfile_cache.insert (cached_getfile_t (k, f));
  }

  void oc_t::output_set_func (penv_t *e, const pfile_set_func_t *s)
  {
    s->output_runtime (e);
    ptr<aarr_t> a = s->get_aarr ();
    const nvtab_t *t = a->nvtab ();
    for (const nvpair_t *p = t->first (); p ; p = t->next (p)) {
      _dest->add (New nvpair_t (p->nm, p->value ()->flatten (e)));
    }
  }

  void oc_t::output_err (penv_t *e, const str &s, int l) 
  {
    warn << "Conf file eval error (" << e->loc (l) << "): " << s << "\n";
  }

  TAMED void 
  ap_t::run_cfg_full (pfnm_t nm, status_cb_t cb, aarr_t *dp)
  {
    VARS {
      output_conf2_t o;
      penv_state_t *st;
      status_t status;
      bool XXX;
    }
    if (!dp) 
      dp = &_base_cfg;
    o.set_dest (dp);
    st = _genv.start_output (NULL, 0);
    BLOCK { publish (&o, nm, &_genv, 0, @(status)); }
    XXX = _genv.finish_output (st);
    SIGNAL (cb, status);
  }

  TAMED void 
  ap_t::run_cfg (pfnm_t nm, cbb cb, aarr_t *dp)
  {
    VARS { status_t status; }
    BLOCK { run_cfg_full (nm, @(status), dp); }
    SIGNAL (cb, status.status == XPUB_STATUS_OK);
  }

  void
  ap_t::list_files_to_check (const pfnm_t &nm, vec<str> *out)
  {
    str d,f;
    bool abs = basename_dirname (nm, &d, &f);
    if (!f) return;
    
    if (abs) {
      out->push_back (nm);
    } else {
      
      strbuf b;
      if (is_remote ()) b << "/";
      b << nm;
      out->push_back (b);

      if (_cwd) 
	out->push_back (apply_container_dir (_cwd, nm));
    }
  }

  void
  lp_t::getfile (pfnm_t fn2, getfile_cb_t cb,
		 const xpub2_file_freshcheck_t &fresh, u_int opts)
  {
    pfnm_t fn3 = _parser->jail2real (fn2);
    phashp_t hsh;
    ptr<bound_pfile2_t> f;
    time_t ctime;
    xpub_status_t res (XPUB_STATUS_OK);
    int opt_mask (P_WSS | P_NOPARSE);
    
    struct stat sb;
    
    // Store the jailed file names in the cache!
    if (!_lookup->lookup (fn2, &hsh, &ctime)) {

      // This might need to go over the network
      hsh = file2hash (fn3, &sb);
      
      // pass 'timenow' (not sb.st_ctime) as the ctime of the file, since 
      // this file is the most up-to-date as of right now.
      ctime = okwstime ();
      _lookup->cache_lookup (fn2, fn3, hsh, timenow, sb.st_size);
    }
    
    if (hsh) {
      
      // It could be that the asked for a new file only if we have one
      // fresher than the given arguments.
      if ((fresh.mode == XPUB2_FRESH_CTIME && ctime < time_t (*fresh.ctime)) ||
	  (fresh.mode == XPUB2_FRESH_HASH  && *hsh == *fresh.hash)) {
	res.set_status (XPUB_STATUS_NOCHANGE);
	
      } else {
	
	pubstat_t s;
	str em;

	ptr<pbinding_t> bnd = New refcounted<pbinding_t> (fn2, hsh);
	if (!_lookup->getfile (bnd->hash (), opts, &f, &s, &em)) {

	  // this might need to go over the network, too,
	  // if we're bypassing NFS
	  if ((f = _parser->pub2_parse (bnd, opts & opt_mask, &s, &em))) {
	    f->set_ctime (ctime);
	    _lookup->cache_getfile (bnd->hash (), opts, f, s, em);
	  }
	}
	
	if (!f) {
	  assert (s != PUBSTAT_OK);
	  res.set_status (pub_stat2status (s));
	  if (em) *res.error = em;
	}
      }
    } else {
      res.set_status (XPUB_STATUS_NOENT);
    }
    (*cb) (res, f);
  }

  bool 
  remote_publisher_t::prepare_getfile (const cache_key_t &k, 
				       xpub2_getfile_arg_t *arg, 
				       ptr<bound_pfile2_t> *f,
				       status_t *status)
  {
    arg->filename = k.fn ();
    arg->options = k.opts();
    arg->fresh.set_mode (XPUB2_FRESH_NONE);

    // XXX please fix
    arg->maxsz = max<int> (ok_axprt_ps - 0x200, 0x1000);
    return false;
  }
  
}

TAMED void 
bound_pfile_t::publish (pub2_iface_t *iface, output_t *o,
			penv_t *genv, pub2::status_cb_t cb) const
{
  VARS {
    bpfcp_t rct;
    pub2::status_t status;
  }
  rct = mkref (const_cast<bound_pfile_t *> (this));

  if (!genv->i_stack_add (rct)) {
    strbuf msg;
    msg << "circular include detected trying to include file "
	<< rct->bnd->filename ();
    o->output_err_stacktrace (genv, msg);
  } else {
    BLOCK { file->publish (iface, o, genv, @(status)); }
    genv->i_stack_remove (rct);
  }

  SIGNAL (cb, status);
}

TAMED template<class T, clist_entry<T> T::*field> void
publist_t<T,field>::publish (pub2_iface_t *iface, output_t *o,
			     penv_t *env, xpub_status_cb_t cb) const
{
  VARS {
    xpub_status_t status (XPUB_STATUS_OK), status_tmp;
    T *e;
  }
  for (e = clist_t<T,field>::first; e; e = next (e)) {
    if (!e->publish_nonblock (iface, o, env)) {
      BLOCK { e->publish (iface, o, env, @(status_tmp)); }
      if (status_tmp.status != XPUB_STATUS_OK)
	status = status_tmp;
    }
  }
  SIGNAL (cb, status);
}
					   
void 
pfile_t::publish (pub2_iface_t *iface, output_t *o, penv_t *genv,
		  pub2::status_cb_t callercv, ptr<closure_t> dummy) const
{
  o->output_header (genv);
  secs.publish (iface, o, genv, callercv);
}

void 
pfile_sec_t::publish (pub2_iface_t *iface, output_t *o, penv_t *genv,
		      pub2::status_cb_t callercv, ptr<closure_t> dummy) const
{
  if (els) 
    els->publish (iface, o, genv, callercv);
  else
    (*callercv) (XPUB_STATUS_OK);
}

void 
pfile_include_t::publish (pub2_iface_t *iface, output_t *o,
			  penv_t *genv, xpub_status_cb_t callercv,
			  ptr<closure_t> dummy) const
{
  if (!fn) {
    o->output_err (genv, "include: no file given", lineno);
    xpub_status_t status (XPUB_STATUS_NOENT);
    *status.error = "include command with no file given";
    (*callercv) (status);
  } else {
    pfile_func_t::publish (iface, o, genv, env, fn, callercv);
  }
}

str
pfile_include2_t::eval_fn (penv_t *e) const
{
  str r;
  if (fn_v2) 
    r = fn_v2->eval (e, EVAL_INTERNAL, true);
  return r;
}

void
pfile_include2_t::publish (pub2_iface_t *iface, output_t *o,
			   penv_t *genv, xpub_status_cb_t callercv,
			   ptr<closure_t> dummy) const
{
  str v = eval_fn (genv);
  if (!v || v.len () == 0) {
    o->output_err (genv, "include: no file given or expansion failed", lineno);
    xpub_status_t status (XPUB_STATUS_NOENT);
    *status.error = "include v2 command with no filename";
    (*callercv) (status);
  } else {
    pfile_func_t::publish (iface, o, genv, env, v, callercv);
  }
}

bool
pfile_include2_t::publish_nonblock (pub2_iface_t *iface, output_t *o,
				    penv_t *genv) const
{
  str v = eval_fn (genv);
  if (!v || v.len () == 0) {
    o->output_err (genv, "include: no file given or expansion failed", lineno);
    return true;
  } else {
    return false;
  }
}

bool
pfile_include_t::publish_nonblock (pub2_iface_t *iface, output_t *o,
				   penv_t *genv) const
{
  if (!fn) {
    o->output_err (genv, "include: no file given", lineno);
    return true;
  } else {
    return false;
  }
}

TAMED void 
pfile_func_t::publish (pub2_iface_t *iface, output_t *o,
		       penv_t *g, aarr_t *e, pfnm_t nm,
		       xpub_status_cb_t cb) const
{
  VARS {
    bool tlf;
    xpub_status_t status (XPUB_STATUS_OK);
  }

  if (o->descend ()) {
    push_frame (g, e);
    tlf = g->set_tlf (false);
    o->output_info (g, strbuf ("include: ") << nm, lineno);
    BLOCK { iface->publish (o, nm, g, lineno, @(status)); }
    o->output_info (g, strbuf ("/include:" ) << nm, lineno);
    g->set_tlf (tlf);
    pop_frame (o, g);
  }
  SIGNAL (cb, status);
}

bool
pfile_switch_t::publish_nonblock (pub2_iface_t *i, output_t *o, penv_t *g) 
  const
{
  pswitch_env_t *pse = eval_for_output (o, g);

  // Need a file name to do include; if doing include, cache the results
  // of the above computation inside the switch object, so that they'll
  // be available to the blocking version of publish() without redoing
  // all computations.
  if (pse && (pse->fn || pse->nested_env ())) {
    _cached_eval_res = pse;
    _eval_cache_flag = false;
    return false;
  } else {
    // In this case, no need to block, so don't cache the eval output.
    _cached_eval_res = NULL;
    _eval_cache_flag = false;
    return true;
  }
}

void
pfile_switch_t::publish (pub2_iface_t *i, output_t *o, penv_t *g,
			 xpub_status_cb_t callercv, ptr<closure_t> dummy) const
{
  pswitch_env_t *pse = _eval_cache_flag ? 
    _cached_eval_res : eval_for_output (o, g);

  _cached_eval_res = NULL;
  _eval_cache_flag = false;

  if (pse) {
    if (pse->fn) {
      pfile_func_t::publish (i, o, g, pse->env (), pse->fn, callercv);
    } else if (pse->nested_env ()) {
      pse->nested_env ()->publish (i, o, g, callercv);
    } else {
      (*callercv) (xpub_status_t (XPUB_STATUS_OK));
    }
  }
}

