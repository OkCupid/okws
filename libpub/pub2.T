// -*-c++-*-
/* $Id$ */

#include "pub2.h"
#include "okdbg.h"
#include "crypt.h"

namespace pub2 {
  
  file_lookup_t g_file_nocache_lookup;

  typedef abstract_publisher_t ap_t;
  typedef remote_publisher_t rp_t;
  typedef caching_remote_publisher_t crp_t;
  typedef output_conf2_t oc_t;
  typedef local_publisher_t lp_t;
  typedef locale_specific_publisher_t lsp_t;

  tamed void
  lsp_t::run_full_T (zbuf *b, pfnm_t fn, getfile_cb_t cb, 
		     aarr_t *a, u_int o, penv_t *e)
  {
    tvars {
      ptr<const pub_localizer_t> old;
      status_t status (XPUB_STATUS_OK);
      ptr<bound_pfile2_t> bpf;
    }
    if (!e) e = _ap->get_env ();
    old = e->localizer ();
    e->set_localizer (_localizer);
    twait { _ap->run_full (b, fn, mkevent (status, bpf), a, o, e); }
    e->set_localizer (old);
    TRIGGER (cb, status, bpf);
  }

  tamed void
  lsp_t::run_T (zbuf *b, pfnm_t fn, cbb cb, 
		aarr_t *a, u_int o, penv_t *e)
  {
    tvars {
      ptr<const pub_localizer_t> old;
      bool ok;
    }
    if (!e) e = _ap->get_env ();
    old = e->localizer ();
    e->set_localizer (_localizer);
    twait { _ap->run (b, fn, mkevent (ok), a, o, e); }
    e->set_localizer (old);
    TRIGGER (cb, ok);
  }

  tamed void
  lsp_t::run_cfg_full_T (pfnm_t nm, getfile_cb_t cb, aarr_t *dest)
  {
    tvars {
      ptr<const pub_localizer_t> old;
      status_t status;
      ptr<bound_pfile2_t> bpf;
      penv_t *e;
    }
    if (!e) e = _ap->get_env ();
    old = e->localizer ();
    e->set_localizer (_localizer);
    twait { _ap->run_cfg_full (nm, mkevent (status, bpf), dest); }
    e->set_localizer (old);
    TRIGGER (cb, status, bpf);
  }

  tamed void
  lsp_t::run_cfg_T (pfnm_t nm, cbb cb, aarr_t *dest)
  {
    tvars {
      status_t status;
      ptr<bound_pfile2_t> dummy;
    }
    twait { run_cfg_full (nm, mkevent (status, dummy), dest); }
    TRIGGER (cb, status.status == XPUB_STATUS_OK);
  }

  tamed void
  ap_t::publish_T (output_t *o, pfnm_t fn, penv_t *env, int lineno,
		 status_cb_t cb)
  {
    tvars {
      status_t status;
      ptr<bound_pfile2_t> dummy;
    }
    twait { publish_full_T (o, fn, env, lineno, mkevent (status, dummy)); }
    TRIGGER (cb, status);
  }

  tamed void 
  ap_t::publish_full_T (output_t *o, pfnm_t fn, penv_t *env, int lineno,
			getfile_cb_t cb)
  {
    tvars {
      status_t status (XPUB_STATUS_OK);
      ptr<bound_pfile2_t> bpf;
      str err;
      vec<str> files_to_check;
      size_t i;
      str d,f, cwd_prev;
      ptr<const pub_localizer_t> localizer;
    }

    if (env && !(o->opts () & P_NOLOCALE)) {
      localizer = env->localizer ();
    }

    list_files_to_check (fn, &files_to_check, localizer);

    if (files_to_check.size () == 0) {
      status.set_status (XPUB_STATUS_NOENT);
      TRIGGER (cb, status, NULL);
      return;
    }

    for (i = 0; i < files_to_check.size (); i++) {
      twait { getfile (files_to_check[i], mkevent (status, bpf), o->opts ()); }
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

      twait { bpf->bpf ()->publish (this, o, env, mkevent (status)); }
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
    
    TRIGGER (cb, status, bpf);
  }
  
  tamed void 
  ap_t::run_T (zbuf *b, pfnm_t nm, cbb cb, aarr_t *a, 
	       u_int op, penv_t *e)
  {
    tvars { 
      status_t status; 
      ptr<bound_pfile2_t> dummy; 
    }
    twait { run_full (b, nm, mkevent (status, dummy), a, op, e); }
    TRIGGER (cb, status.status == XPUB_STATUS_OK);
  }
  
  tamed void 
  ap_t::run_full_T (zbuf *b, pfnm_t nm, getfile_cb_t cb,
		    aarr_t *a, u_int op, penv_t *env)
  {
    tvars {
      ptr<output_std_t> o;
      penv_state_t *st;
      status_t status;
      bool XXX;
      pfile_type_t typ;
      ptr<bound_pfile2_t> bpf;
      penv_t *env_local;
    }
    if (op == 0)
      op = _opts;

    // The other possibility is to lock env via tame locking.
    // If this copy constructor turns out to be expensive in terms of CPU,
    // we can try that.  Put a lock object in env, and since this
    // function isn't called recursively, we don't need to worry
    // about reacquring the lock recursively. Would also allow a rollback
    // from SVN version 2182 to 2179.
    env_local = env ? New penv_t (*env) : New penv_t (_genv);

    // 
    // Running with GLOBALSET allows <!--#set --> to behave 'globally'
    // and not to make only local changes to the evironment variable
    // stack.
    //
    op |= P_GLOBALSET;

    typ = (op & P_WSS) ? PFILE_TYPE_WH : PFILE_TYPE_H;
    o = New refcounted<output_std_t> (b, typ, op);
    st = env_local->start_output (a, op);
    twait { publish_full (o, nm, env_local, 0, mkevent (status, bpf)); }
    
    // XXX = ignore output from the following function
    XXX = env_local->finish_output (st);
    delete env_local;

    TRIGGER (cb, status, bpf);
  }

  tamed void 
  rp_t::connect (cbb cb)
  {
    tvars {
      clnt_stat err;
      bool ret (true);
    }
    twait { _cli->call (PUB2_NULL, NULL, NULL, mkevent (err)); }
    if (err) {
      warn << "NULL call to pubd failed: " << err << "\n";
      ret = false;
    }
    TRIGGER (cb, ret);
  }

  tamed void 
  crp_t::connect (cbb cb)
  {
    tvars { 
      clnt_stat stat;
      bool ret (true);
    }
    twait { _cli->call (PUB2_GET_PUSHES, NULL, &_connected, mkevent (stat)); }
    if (stat) {
      warn << "Intial RPC call to pubd-2 failed: " << stat << "\n";
      ret = false;
    } else if (!_connected) {
      warn << "pubd-2 denied request for file status pushes!\n";
    }
    TRIGGER (cb, ret);
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

  bool
  crp_t::is_cached (const pfnm_t &n, u_int o, const phash_t &h) const
  {
    cache_key_t k (n, o);
    const cached_getfile_t *c = _getfile_cache[k];
    return c && h == *(c->file ()->hash ());
  }

  tamed static void
  wait_for_nsignals (u_int n, rendezvous_t<ptr<bool> > *cg, cbb cb)
  {
    tvars {
      ptr<bool> tmpok;
      bool ok (true);
    }
    while (cg->n_triggers_left () > n) {
      twait (*cg, tmpok);
      if (!*tmpok) ok = false;
    }
    TRIGGER (cb, ok);
  }

  tamed void
  rp_t::getchunk (const xpub2_getfile_data_t *dat, u_int opts, size_t offset, 
		  size_t sz, char *buf, cbb cb)
  {
    tvars {
      clnt_stat err;
      xpub2_getchunk_arg_t arg;
      xpub2_getchunk_res_t res;
      bool ok (false);
      strbuf b;
    }
    memcpy (arg.hash.base (), dat->stat.hash.base (), PUBHASHSIZE);
    arg.offset = offset;
    arg.size = sz;
    arg.opts = opts;

    twait { _cli->call (PUB2_GETCHUNK, &arg, &res, mkevent (err) ); }

    if (err) {
      b << "RPC error in PUB2_GETCHUNK: " << err << "\n";
      OKDBG3(PUB2_CHUNKS, ERROR, b);

    } else if (res.status != XPUB_STATUS_OK) {
      b << "PUB error in PUB2_GETCHUNK: " << res.status;
      if (res.status == XPUB_STATUS_ERR)
	b << ": " << *res.error;
      OKDBG3(PUB2_CHUNKS, ERROR, b);

    } else if (res.chunk->data.size () != sz) {
      OKDBG3(PUB2_CHUNKS, ERROR, 
	     "Error in PUB2_GETCHUNK: datasize mismatch");

    } else {
      ok = true;
      memcpy (buf + offset, res.chunk->data.base (), res.chunk->data.size ());

    }
    TRIGGER (cb, ok);
  }

  tamed void
  rp_t::launch_getchunk (rendezvous_t<ptr<bool> > *cg, 
			 const xpub2_getfile_data_t *dat, u_int opts,
			 size_t offset, size_t dsz, char *buf, cbb cb)
  {
    tvars {
      u_int ws (5);
      bool ok (true);
      ptr<bool> nok;
    }

    assert (ws > 0);

    twait { wait_for_nsignals (ws - 1, cg, mkevent (ok)); }
    if (ok) {
      nok = New refcounted<bool> ();
      getchunk (dat, opts, offset, dsz, buf, mkevent (*cg, nok, *nok));
    }

    TRIGGER (cb, ok);
  }

  tamed void
  rp_t::getfile_chunked (const xpub2_getfile_data_t *dat, u_int opts,
			 xpub_file_t *file, status_cb_t cb)
  {
    tvars {
      status_t status (XPUB_STATUS_OK);
      size_t dsz (dat->file.chunked->datasize);
      mstr buf (dsz);
      rendezvous_t<ptr<bool> > cg (__FILE__, __LINE__);
      bool ok (true), tmpok;
      str m;
      size_t cs;
      size_t i;
    }

    for (i = 0; i < dsz && ok; i += _maxsz) {
      cs = min<size_t> (_maxsz, dsz - i);
      twait { 
	launch_getchunk (&cg, dat, opts, i, cs, buf.cstr (), mkevent (tmpok)); 
      }
      if (!tmpok) ok = false;
    }
    twait { wait_for_nsignals (0, &cg, mkevent (tmpok)); }
    if (!tmpok) ok = false;

    if (!ok) {
      m = "Not all chunks retrieved"; 
    } else {
      char h[PUBHASHSIZE];
      sha1_hash (h, buf.cstr (), buf.len ());
      if (memcmp (h, dat->file.chunked->xdrhash.base (), PUBHASHSIZE)) {
	m = "Hash mismatch";
      } else if (!str2xdr (*file, buf)) {
	m = "Demarshall of chunks failed";
      }
    }

    if (m) {
      status.set_status (XPUB_STATUS_CORRUPTION);
      *status.error = m;
    }

    TRIGGER (cb, status);
  }

  tamed void
  rp_t::getfile_body (pfnm_t nm, const xpub2_getfile_res_t *res, 
		      getfile_cb_t cb, u_int opts)
  {
    tvars {
      ptr<bound_pfile2_t> bpf;
      status_t status (XPUB_STATUS_OK);
      const xpub_file_t *filep (NULL);
      xpub_file_t file;
    }
    if (res->data->file.mode == XPUB_XFER_WHOLE) {
      filep = res->data->file.whole;
    } else if (res->data->file.mode == XPUB_XFER_CHUNKED) {
      twait { getfile_chunked (res->data, opts, &file, mkevent (status)); }
      filep = &file;
    } else {
      status.set_status (XPUB_STATUS_ERR);
      *status.error = "Unknown transfer mode";
    }
    if (filep)
      bpf = New refcounted<bound_pfile2_t> (nm, res->data->stat, *filep, opts);
    TRIGGER (cb, status, bpf);
  }

  tamed void 
  rp_t::getfile_T (pfnm_t nm, getfile_cb_t cb, u_int opts)
  {
    tvars {
      clnt_stat err;
      ptr<bound_pfile2_t> file;
      xpub2_getfile_arg_t arg;
      xpub2_getfile_res_t res;
      status_t status (XPUB_STATUS_OK);
      cache_key_t k (nm, opts);
    }
    if (!prepare_getfile (k, &arg, &file, &status)) {
      twait { _cli->call (PUB2_GETFILE, &arg, &res, mkevent (err)); }
      if (err) {
	warn << nm << ": error in RPC call to PUB2_GETFILE: " 
	     << err << "\n";
	status.set_status(XPUB_STATUS_RPC_ERR);
	*status.rpc_err = err;
      } else {
	switch (res.status) {
	case XPUB_STATUS_OK:
	  twait { getfile_body (nm, &res, mkevent (status, file), opts); }
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

	    // it's OK to fail here, since we can be checking for many
	    // files in the path.
	    // warn << b << "\n";

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
    TRIGGER (cb, status, file);
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
    } else {
      if (OKDBG2(PUB2_CACHE)) {
	str tmp = k.fn ();
	if (!tmp) tmp = "<NONE>";
	okdbg_warn (CHATTER, "cache MISS: %s:%x\n", tmp.cstr (), k.opts ());
      }
      return rp_t::prepare_getfile (k, arg, f, status);
    }
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

    OKDBG4(PUB2_CACHE, CHATTER, 
	   "cache INSERT: %s:%x\n", k.fn ().cstr (), k.opts ());

    _noent_cache.remove (k.fn ());

    _getfile_cache.insert (cached_getfile_t (k, f));
  }

  void oc_t::output_set_func (penv_t *e, const pfile_set_func_t *s)
  {
    s->output_runtime (e);
    ptr<aarr_t> a = s->get_aarr ();
    nvtab_t *t = a->nvtab ();
    for (nvpair_t *p = t->first (); p ; p = t->next (p)) {
      _dest->add (New nvpair_t (p->nm, p->value ()->flatten (e)));
    }
  }

  void oc_t::output_err (penv_t *e, const str &s, int l) 
  {
    warn << "Conf file eval error (" << e->loc (l) << "): " << s << "\n";
  }

  tamed void 
  ap_t::run_cfg_full_T (pfnm_t nm, getfile_cb_t cb, aarr_t *dp)
  {
    tvars {
      output_conf2_t o;
      penv_state_t *st;
      status_t status;
      bool XXX;
      ptr<bound_pfile2_t> bpf;
      penv_t *env (NULL);
      bool del_env (false);
      u_int op (0);
    }

    if (!dp) {

      // In the first case, just publish the data to the global publishing
      // object, in which case all changes are effectively global, and
      // we don't need to worry about race conditions, since it's all
      // going to the same place.
      dp = &_base_cfg;
      env = &_genv;
    } else {

      // In the second case, we're publishing to a given array, in which
      // case we are worried about race conditions.  We also want the behavior
      // in which the changes to the environment produced by <!--#set -->
      // persist past the end of the include, and therefore, we need
      // to set the GLOBALSET flag.  Of course, all of this should be
      // a lot cleaner, but for now, this shouldn't break anything else
      // and it should work.
      env = New penv_t (_genv);
      dp->overwrite_with (_base_cfg);
      del_env = true;
      op |= P_GLOBALSET;
    }

    o.set_dest (dp);
    st = env->start_output (NULL, op);
    twait { publish_full (&o, nm, env, 0, mkevent (status, bpf)); }
    XXX = env->finish_output (st);

    if (del_env)
      delete env;

    TRIGGER (cb, status, bpf);
  }

  tamed void 
  ap_t::run_cfg_T (pfnm_t nm, cbb cb, aarr_t *dp)
  {
    tvars { 
      status_t status; 
      ptr<bound_pfile2_t> dummy;
    }
    twait { run_cfg_full (nm, mkevent (status, dummy), dp); }
    TRIGGER (cb, status.status == XPUB_STATUS_OK);
  }

  void
  ap_t::list_files_to_check (const pfnm_t &nm, vec<str> *out, 
			     ptr<const pub_localizer_t> l)
  {
    str d,f;
    bool abs = basename_dirname (nm, &d, &f);
    if (!f) return;
    
    if (abs) {
      if (l) out->push_back (l->localize (nm));
      out->push_back (nm);
    } else {
      
      str n1, n2;
      strbuf b;
      if (is_remote ()) b << "/";
      b << nm;
      n1 = b;
      
      if (_cwd) {
	n2 = apply_container_dir (_cwd, nm);
      }
      if (l) {
	if (n1) out->push_back (l->localize (n1));
	if (n2) out->push_back (l->localize (n2));
      }
      if (n1) out->push_back (n1);
      if (n2) out->push_back (n2);
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
      _lookup->cache_lookup (fn2, fn3, hsh, sfs_get_timenow(), sb.st_size);
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
    arg->maxsz = _maxsz;
    return false;
  }
  
}

tamed void 
bound_pfile_t::publish (pub2_iface_t *iface, output_t *o,
			penv_t *genv, pub2::status_cb_t cb) const
{
  tvars {
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
    twait { file->publish (iface, o, genv, mkevent (status)); }
    genv->i_stack_remove (rct);
  }

  TRIGGER (cb, status);
}

tamed template<class T, clist_entry<T> T::*field> void
publist_t<T,field>::publish (pub2_iface_t *iface, output_t *o,
			     penv_t *env, xpub_status_cb_t cb) const
{
  tvars {
    xpub_status_t status (XPUB_STATUS_OK), status_tmp;
    T *e;
  }
  for (e = clist_t<T,field>::first; e; e = next (e)) {
    if (!e->publish_nonblock (iface, o, env)) {
      twait { e->publish (iface, o, env, mkevent (status_tmp)); }
      if (status_tmp.status != XPUB_STATUS_OK)
	status = status_tmp;
    }
  }
  TRIGGER (cb, status);
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

tamed void 
pfile_func_t::publish (pub2_iface_t *iface, output_t *o,
		       penv_t *g, aarr_t *e, pfnm_t nm,
		       xpub_status_cb_t cb) const
{
  tvars {
    bool tlf;
    xpub_status_t status (XPUB_STATUS_OK);
    bool muzzled;
  }

  if (o->descend ()) {

    muzzled = o->set_muzzle (muzzle_output ());

    push_frame (g, e);
    tlf = g->set_tlf (false);
    o->output_info (g, strbuf ("include: ") << nm, lineno);
    twait { iface->publish (o, nm, g, lineno, mkevent (status)); }
    o->output_info (g, strbuf ("/include:" ) << nm, lineno);
    g->set_tlf (tlf);
    pop_frame (o, g);

    o->set_muzzle (muzzled);
  }
  TRIGGER (cb, status);
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

