// -*-c++-*-

#include "pub3hilev.h"
#include "pub3prot.h"
#include "crypt.h"
#include "pub3hilev.h"
#include "pub3out.h"

static str
trim_null_bytes (str s)
{
  const char *c = s.cstr();
  if (s.len () != strlen (c)) s = c;
  return s;
}

namespace pub3 {

  typedef abstract_publisher_t ap_t;
  typedef remote_publisher_t rp_t;
  typedef caching_remote_publisher_t crp_t;
  typedef local_publisher_t lp_t;

  //=======================================================================

  static void
  status2evec (const xpub_status_t &res, vec<str> *out) 
  {
    if (res.status == XPUB_STATUS_RPC_ERR) {
      strbuf b;
      b << "RPC error:" << clnt_stat (*(res.rpc_err));
      out->push_back (b);
    } else if (res.status == XPUB_STATUS_NOENT ||
	       res.status == XPUB_STATUS_EPARSE || 
	       res.status == XPUB_STATUS_EIO) {
      for (size_t i = 0; i < res.errors->size (); i++) {
	out->push_back ((*res.errors)[i]);
      }
    } else {
      out->push_back (*res.error);
    }
  }

  //=======================================================================

  bool 
  localizer_t::localize_many (const str &infn, vec<str> *out) const 
  { return false; }

  //-----------------------------------------------------------------------

  str localizer_t::localize (const str &infn) const { return NULL; }

  //-----------------------------------------------------------------------

  void
  localizer_t::compound_localize (const str &infn, vec<str> *out) const
  {
    str res;
    
    if (!localize_many(infn, out))
      res = localize(infn);
    if (res)
      out->push_back(res);
  }

  //=======================================================================

  abstract_publisher_t::abstract_publisher_t (opts_t o, 
					      ptr<const localizer_t> l) : 
    _opts (o), 
    _pub3_err_obj_key (ok_pub3_err_obj_key),
    _localizer (l) {}

  //-----------------------------------------------------------------------

  ptr<expr_dict_t> ok_iface_t::get_universals () 
  { return singleton_t::get ()->universals (); }

  //-----------------------------------------------------------------------

  obj_t ok_iface_t::get_universals_obj () 
  { return pub3::obj_dict_t (get_universals ()); }

  //-----------------------------------------------------------------------

  obj_dict_t ok_iface_t::pub3_config_obj ()
  { return pub3::obj_dict_t (get_universals ()); }

  //-----------------------------------------------------------------------

  void
  ap_t::init_for_run (eval_t *p, opts_t opts, ptr<expr_dict_t> d)
  {
    if (opts < 0) opts = _opts;
    
    p->set_opts (opts);
    p->set_pub_iface (mkref (this));

    if (d) {
      p->env ()->bind_globals (d);
    }

    // In "placeholders" mode, there's a little red placeholder published
    // and then the error object is published into the page, for the 
    // designer to style as desired.
    if ((opts & P_OUTPUT_ERR_OBJ) && _pub3_err_obj_key) {
      p->add_err_obj (_pub3_err_obj_key);
    }
  }

  //-----------------------------------------------------------------------

  void
  ap_t::uninit_for_run (eval_t *p)
  {
    p->set_pub_iface (NULL);
  }

  //-----------------------------------------------------------------------
  
  tamed void 
  ap_t::run (zbuf *b, str fn, evb_t ev, ptr<expr_dict_t> d, opts_t opts, 
	     status_t *sp, ptr<file_t> *fp)
  {
    tvars {
      holdvar ptr<bindtab_t> unis (singleton_t::get ()->universals ());
      ptr<eval_t> pub (New refcounted<eval_t> (unis, b, opts));
      status_t status;
      ptr<file_t> file;
    }

    init_for_run (pub, opts, d);
    
    twait { publish (pub, fn, mkevent (status, file)); }

    if (sp) *sp = status;
    if (fp) *fp = file;

    uninit_for_run (pub);

    ev->trigger (status.status == XPUB_STATUS_OK);
  }

  //-----------------------------------------------------------------------

  tamed void
  ap_t::run_cfg (str fn, evb_t ev, ptr<expr_dict_t> d_in, opts_t o,
		 status_t *sp)
  {
    tvars {
      holdvar ptr<bindtab_t> unis (singleton_t::get ()->universals ());
      ptr<expr_dict_t> d (d_in ? d_in : expr_dict_t::alloc ());
      opts_t opts ((o < 0 ? _self->opts () : o) | P_CONFIG);
      ptr<eval_t> pub (New refcounted<eval_t> (unis, d, opts));
      status_t status;
      ptr<file_t> file;
    }
    pub->set_pub_iface (mkref (this));

    twait { publish (pub, fn, mkevent (status, file)); }

    if (sp) *sp = status;
    
    pub->set_pub_iface (NULL);

    // Finally, if requested, copy the universal pub information
    // back into the universals slot...
    if (!d_in || (opts & P_COPY_CONF)) {
      *(singleton_t::get ()->universals ()) += *d;
    }
    
    ev->trigger (status.status == XPUB_STATUS_OK);
  }
  
  //-----------------------------------------------------------------------
  
  tamed void 
  ap_t::publish_prepare (eval_t *pub, str fn, str *rfn, str *errp, 
                         getfile_ev_t ev)
  {
    tvars {
      status_t status (XPUB_STATUS_NOENT);
      ptr<file_t> file;
      str err;
      vec<str> files_to_check;
      size_t i;
      ptr<const localizer_t> localizer;
    }

    // generate a list of internationalized files.  if that list is
    // empty, we'll skip the following loop and we'll just
    // fall through with status.status == XPUB_STATUS_NOENT
    localizer = get_localizer (pub);
    list_files_to_check (pub->cwd (), fn, &files_to_check, localizer);

    pub->set_active(false);
    for (i = 0; i < files_to_check.size (); i++) {
      twait {
          getfile (files_to_check[i], mkevent (status, file), pub->opts ());
      }
      if (status.status != XPUB_STATUS_NOENT) {
          break;
      }
    }
    pub->set_active(true);

    switch (status.status) {
    case XPUB_STATUS_OK:
      assert (file);
      assert (i < files_to_check.size ());
      *rfn = files_to_check[i];
      break;
    case XPUB_STATUS_RPC_ERR:
      err = strbuf () << fn << ": RPC error: " << *status.rpc_err;
      break;
    case XPUB_STATUS_NOENT:
      err = strbuf () << fn << ": file not found";
      break;
    case XPUB_STATUS_EIO:
    case XPUB_STATUS_EPARSE:
      pub->output_errs (*status.errors, P_ERR_ERROR);
      break;
    default:
      err = *status.error;
      break;
    }
    *errp = err;

    ev->trigger (status, file);
  }

  //-----------------------------------------------------------------------

  tamed void
  ap_t::publish (eval_t *pub, str fn, getfile_ev_t ev)
  {
    tvars {
      status_t status (XPUB_STATUS_NOENT);
      ptr<file_t> file;
      str err;
      str d, f, cwd_prev, rfn;
    }

    twait { publish_prepare (pub, fn, &rfn, &err, mkevent (status, file)); }
    if (status.status == XPUB_STATUS_OK) {
      assert (file);
      file->metadata ()->set_input_filename (fn);
      basename_dirname (rfn, &d, &f);

      cwd_prev = pub->set_cwd (d);

      twait { pub->publish_file (file, mkevent (status)); }
      if (status.status != XPUB_STATUS_OK)  {
	err = strbuf () << "error in publishing file: " << fn;
      }

      // Reset the working directory to whatever it was previously.
      pub->set_cwd (cwd_prev);
    }
    if (err) { pub->output_err (err, P_ERR_ERROR); }
    ev->trigger (status, file);
  }
  
  //-----------------------------------------------------------------------
  
  static void use_file (str f, vec<str> *out) { if (f) out->push_back (f); }
  
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
      if (l)        l->compound_localize (nm, out); 
      if (use_defs) use_file (nm, out);
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
	if (n1) l->compound_localize (n1, out);
	if (n2) l->compound_localize (n2, out);
      }
 
      if (use_defs) {
	if (n1) use_file (n1, out);
	if (n2) use_file (n2, out);
      }
    }

    if (l) { use_file (l->get_default_fnf (), out); }
  }

  //-----------------------------------------------------------------------

  ptr<const localizer_t> 
  ap_t::get_localizer (eval_t *p)
  {
    ptr<const localizer_t> ret;
    if (!(p->opts () & P_NOLOCALE)) {
      ret = _localizer;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  tamed void 
  ap_t::syntax_check (str f, vec<str> *err, evi_t ev)
  {
    tvars {
      xpub_status_t res;
      ptr<file_t> dummy;
      int rc (0);
    }
    twait { getfile (f, mkevent (res, dummy), 0); }
    if (res.status != XPUB_STATUS_OK) {
      status2evec (res, err);
      rc = -1;
    } else {
      rc = 0;
    }
    ev->trigger (rc);
  }
  
  //=======================================================================
  
  remote_publisher_t::remote_publisher_t (ptr<axprt_stream> x, opts_t o)
    : ap_t (o),
      _x (x),
      _cli (aclnt::alloc (x, pub_prog_3)),
      _srv (asrv::alloc (x, pub_prog_3, wrap (this, &rp_t::dispatch))),
      _maxsz (min<size_t> (ok_pub3_max_datasz, ok_axprt_ps >> 1)) {}

  //-----------------------------------------------------------------------
  
  tamed void 
  rp_t::connect (evb_t ev)
  {
    tvars {
      clnt_stat err;
      bool ret (true);
    }
    twait { rpc::pub_prog_3::pub3_null (_cli, mkevent (err)); }
    if (err) {
      warn << "NULL call to pubd failed: " << err << "\n";
      ret = false;
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------
  
  void rp_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      lost_connection ();
      warn << "connection to pubd unexpectedly died!\n";
      exit (0);
    } else {
      switch (sbp->proc ()) {
      case PUB3_PUSH_DELTAS:
	handle_new_deltas (sbp);
	break;
      default:
	sbp->reject (PROC_UNAVAIL);
	break;
      }
    }
  }

  //-----------------------------------------------------------------------
  
  tamed void
  rp_t::getchunk (const xpub3_hash_t &key, opts_t opts, size_t offset, 
		  size_t sz, char *buf, evb_t ev)
  {
    tvars {
      clnt_stat err;
      xpub3_getchunk_arg_t arg;
      xpub3_getchunk_res_t res;
      bool ok (false);
      strbuf b;
    }
    memcpy (arg.hash.base (), key.base (), PUBHASHSIZE);
    arg.offset = offset;
    arg.size = sz;
    arg.opts = opts;

    twait { rpc::pub_prog_3::pub3_getchunk (_cli, arg, &res, mkevent (err) ); }

    if (err) {
      b << "RPC error in PUB3_GETCHUNK: " << err << "\n";
      OKDBG3(PUB3_CHUNKS, ERROR, b);

    } else if (res.status != XPUB_STATUS_OK) {
      b << "PUB error in PUB3_GETCHUNK: " << res.status;
      if (res.status == XPUB_STATUS_ERR)
	b << ": " << *res.error;
      OKDBG3(PUB3_CHUNKS, ERROR, b);

    } else if (res.chunk->data.size () != sz) {
      OKDBG3(PUB3_CHUNKS, ERROR, 
	     "Error in PUB3_GETCHUNK: datasize mismatch");
      
    } else {
      ok = true;
      memcpy (buf + offset, res.chunk->data.base (), res.chunk->data.size ());

      if (OKDBG2(PUB3_CHUNKS)) {
	strbuf b ("get_chunk(");
	rpc_print (b, arg);
	char buf[PUBHASHSIZE];
	sha1_hash (buf, res.chunk->data.base (), res.chunk->data.size ());
	str fp = armor32 (buf, PUBHASHSIZE);
	b.fmt (") -> %s", fp.cstr ());
	okdbg_warn (CHATTER, b);
      }
    }
    ev->trigger (ok);
  }

  //-----------------------------------------------------------------------
  
  void rp_t::handle_new_deltas (svccb *sbp)
  {
    rpc::pub_prog_3::pub3_push_deltas_srv_t<svccb> srv (sbp);
    srv.reply (false);
  }

  //-----------------------------------------------------------------------
  
  tamed void
  rp_t::getfile_chunked (const xpub3_chunkshdr_t &hdr, opts_t opts,
			 xpub3_file_t *file, status_ev_t ev)
  {
    tvars {
      status_t status (XPUB_STATUS_OK);
      size_t dsz (hdr.datasize);
      mstr buf (dsz);
      rendezvous_t<ptr<bool> > rv (__FILE__, __LINE__);
      bool ok (true);
      str m;
      size_t cs;
      size_t i (0);
      size_t nsent (0), nrecv (0);
      size_t ws (ok_pub3_chunk_window_size);
      ptr<bool> nok;
    }

    memset (buf, 0xcd, dsz);

    for ( ; (i < dsz || nrecv < nsent) && ok;) {

      if (i < dsz && nsent - nrecv < ws) {
	nsent ++;
	cs = min<size_t> (_maxsz, dsz - i);
	nok = New refcounted<bool> (true);

	getchunk (hdr.dathash, opts, i, cs, buf.cstr (), 
		  mkevent (rv, nok, *nok));
	i += _maxsz;
      } else {
	twait (rv, nok);
	nrecv++;
	if (!*nok) ok = false;
      }
    }

    buf.setlen (dsz);

    if (!ok) {
      rv.cancel ();
      m = "Not all chunks retrieved"; 
    } else {
      char h[PUBHASHSIZE];
      sha1_hash (h, buf.cstr (), buf.len ());
      if (memcmp (h, hdr.xdrhash.base (), PUBHASHSIZE) != 0) {
	m = "Hash mismatch";
      } else if (!str2xdr (*file, buf)) {
	m = "Demarshall of chunks failed";
      }
      if (OKDBG2(PUB3_CHUNKS)) {
	  str s1 = armor32 (h, PUBHASHSIZE);
	  str s2 = armor32 (hdr.xdrhash.base (), PUBHASHSIZE);
	  if (m) {
	    okdbg_warn (ERROR, "hash mismatch (%s v %s)\n", 
			s1.cstr (), s2.cstr ());
	  } else {
	    okdbg_warn (CHATTER, "hash match on %s\n", s1.cstr ());
	  }
      }
    }

    if (m) {
      status.set_status (XPUB_STATUS_CORRUPTION);
      *status.error = m;
    }

    ev->trigger (status);
  }

  //-----------------------------------------------------------------------
  
  tamed void
  rp_t::getfile_body (str nm, const xpub3_getfile_res_t *res, 
		      getfile_ev_t ev, opts_t opts)
  {
    tvars {
      ptr<file_t> out;
      status_t status (XPUB_STATUS_OK);
      const xpub3_file_t *filep (NULL);
      xpub3_file_t file;
    }
    if (res->file->mode == XPUB_XFER_WHOLE) {
      filep = res->file->whole;
    } else if (res->file->mode == XPUB_XFER_CHUNKED) {
      twait { 
	getfile_chunked (*res->file->chunked, opts, &file, mkevent (status)); 
      }
      if (status.status == XPUB_STATUS_OK) {
	filep = &file;
      }
    } else {
      status.set_status (XPUB_STATUS_ERR);
      *status.error = "Unknown transfer mode";
    }

    if (filep) { out = file_t::alloc (*filep, opts); }

    ev->trigger (status, out);
  }

  //-----------------------------------------------------------------------
  
  bool 
  rp_t::prepare_getfile (const cache_key_t &k, xpub3_getfile_arg_t *arg, 
			 ptr<file_t> *f, status_t *status)
  {
    // We've had some attacks wherein attackers ask for files of the
    // form "/fooo\0\0\0\0". This causes an XDR failure since filenames
    // are XDR strings and not opaques.  The quick solution is just to
    // trim off any trailing null bytes and stop the string at the first
    // one, upholding the required assertion.
    arg->filename = trim_null_bytes (k.fn ());

    arg->options = k.opts();
    arg->fresh.set_mode (XPUB3_FRESH_NONE);
    arg->maxsz = _maxsz;
    return false;
  }

  //-----------------------------------------------------------------------

  void rp_t::getfile (str fn, getfile_ev_t ev, opts_t o)
  { getfile_T (fn, ev, o); }
  
  //-----------------------------------------------------------------------

  tamed void
  rp_t::getfile_T (str nm, getfile_ev_t ev, opts_t opts)
  {
    tvars {
      clnt_stat err;
      ptr<file_t> file;
      xpub3_getfile_arg_t arg;
      xpub3_getfile_res_t res;
      status_t status (XPUB_STATUS_OK);
      cache_key_t k (nm, opts);
    }
    if (!prepare_getfile (k, &arg, &file, &status)) {
      twait { rpc::pub_prog_3::pub3_getfile(_cli, arg, &res, mkevent (err)); }
      if (err) {
	warn << nm << ": error in RPC call to PUB2_GETFILE: " 
	     << err << "\n";
	status.set_status(XPUB_STATUS_RPC_ERR);
	*status.rpc_err = err;
      } else {

	switch (res.code) {

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
	    status.errors->push_back (b);
	  }
	  break;

	default:
	  status = *res.error_status;
	  break;
	}
      }
    }
    ev->trigger (status, file);
  }

  //=======================================================================

  local_publisher_t::local_publisher_t (ptr<pub_parser_t> p, 
					opts_t opts,
					ptr<file_lookup_t> l,
					ptr<jailer_t> j)
    : abstract_publisher_t (opts),
      _parser (p ? p : pub_parser_t::alloc ()), 
      _lookup (l ? l : file_lookup_t::alloc ()),
      _jailer (j ? j : jailer_t::alloc ()) {}
  
  //-----------------------------------------------------------------------

  void
  lp_t::getfile (str fn, getfile_ev_t ev, opts_t o)
  {
    xpub3_file_freshcheck_t frsh (XPUB3_FRESH_NONE);
    getfile (fn, ev, frsh, o);
  }

  //-----------------------------------------------------------------------

  void
  lp_t::getfile (str jfn, getfile_ev_t ev, 
		 const xpub3_file_freshcheck_t &fresh, opts_t o)
  {
    str rfn = _jailer->jail2real (jfn);

    ptr<fhash_t> hsh;
    ptr<file_t> f;
    time_t ctime;
    xpub_status_t res (XPUB_STATUS_OK);
    opts_t opt_mask (P_NOPARSE);
    
    struct ::stat sb;
    
    // Store the jailed file names in the cache!
    if (!_lookup->lookup (jfn, &hsh, &ctime)) {

      // This might need to go over the network
      hsh = file2hash (rfn, &sb);
      
      // pass 'timenow' (not sb.st_ctime) as the ctime of the file, since 
      // this file is the most up-to-date as of right now.
      time_t now = okwstime ();
      ctime = now;
      _lookup->cache_lookup (jfn, rfn, hsh, now, sb.st_size);
    }
    
    if (hsh) {
      
      // It could be that the asked for a new file only if we have one
      // fresher than the given arguments.
      if ((fresh.mode == XPUB3_FRESH_CTIME && ctime < time_t (*fresh.ctime)) ||
	  (fresh.mode == XPUB3_FRESH_HASH  && *hsh == *fresh.hash)) {

	res.set_status (XPUB_STATUS_NOCHANGE);
	
      } else {
	
	parse_status_t s;
	str em;
	parse_ret_t pr;

	ptr<metadata_t> meta = New refcounted<metadata_t> (jfn, rfn, hsh);
	if (!_lookup->getfile (hsh, o, &f, &s, &em)) {

	  // this might need to go over the network, too,
	  // if we're bypassing NFS
	  if (_parser->parse (meta, &pr, o & opt_mask)) {
	    f = pr.file ();
	    meta->set_ctime (ctime);
	    _lookup->cache_getfile (hsh, o, f, PARSE_OK, NULL);
	  }
	}
	
	if (!pr.ok ()) {
	  assert (!f);
	  pr.to_xdr (&res);
	}
      }
    } else {
      res.set_status (XPUB_STATUS_NOENT);
    }
    ev->trigger (res, f);
  }

  //=======================================================================
  
  bool 
  crp_t::prepare_getfile (const cache_key_t &k, xpub3_getfile_arg_t *arg,
			  ptr<file_t> *f, status_t *status)
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
      status->errors->push_back (b);
      *f = NULL;
      return true;
    } else {
      if (OKDBG2(PUB3_CACHE)) {
	str tmp = k.fn ();
	if (!tmp) tmp = "<NONE>";
	okdbg_warn (CHATTER, "cache MISS: %s:%x\n", tmp.cstr (), k.opts ());
      }
      return rp_t::prepare_getfile (k, arg, f, status);
    }
  }

  //----------------------------------------------------------------------

  tamed void 
  crp_t::connect (evb_t ev)
  {
    tvars { 
      clnt_stat stat;
      bool ret (true);
    }

    twait { 
      rpc::pub_prog_3::pub3_get_pushes (_cli, &_connected, mkevent (stat)); 
    }

    if (stat) {
      warn << "Intial RPC call to pubd failed: " << stat << "\n";
      ret = false;
    } else if (!_connected) {
      warn << "pubd denied request for file status pushes!\n";
    }
    ev->trigger (ret);
  }

  //-----------------------------------------------------------------------

  bool
  crp_t::invalidate_cache (str n) 
  {
    bool ret = false;
    ptr<bhash<opts_t> > *h = _opts_map[n];
    if (h) {

      ret = true;
      assert (*h);
      bhash_const_iterator_t<opts_t> it (**h);
      const opts_t *o;
      while ((o = it.next ())) { rm_file (n, *o); }
      _opts_map.remove (n);

      if (OKDBG2(PUB3_CACHE)) {
	strbuf b;
	b << "Removing file '" << n << "' from GETFILE cache";
	okdbg_warn (CHATTER, b);
      }
    }
    return ret;

  }


  //-----------------------------------------------------------------------
  
  void 
  crp_t::handle_new_deltas (const xpub3_delta_set_t &s)
  {
    if (_delta_id >= 0 && s.serial != _delta_id + 1) {
      okdbg_warn (ERROR, "Missed a delta update! Clearing cache!");
      _opts_map.clear ();
      _getfile_cache.clear ();
    } else {
      _delta_id = s.serial;
      for (size_t i = 0; i < s.files.size (); i++) {
	str n = s.files[i];
	invalidate_cache (n);
      }
    }
  }

  //-----------------------------------------------------------------------

  void
  crp_t::cache_noent (str s)
  {
    _noent_cache.insert (s);
    invalidate_cache (s);
  }
  
  //-----------------------------------------------------------------------
  
  bool
  crp_t::is_cached (str n, opts_t o, const fhash_t &h) const
  {
    cache_key_t k (n, o);
    const cached_getfile_t *c = _getfile_cache[k];
    return c && h == c->file ()->metadata ()->hash ();
  }

  //-----------------------------------------------------------------------
  
  void 
  crp_t::handle_new_deltas (svccb *sbp)
  {
    rpc::pub_prog_3::pub3_push_deltas_srv_t<svccb> srv (sbp);
    const xpub3_delta_set_t *s = srv.getarg ();
    handle_new_deltas (*s);
    srv.reply (true);
  }

  //-----------------------------------------------------------------------

  void crp_t::lost_connection ()
  {
    _connected = false;
    clear_cache ();
  }

  //-----------------------------------------------------------------------
  
  void
  crp_t::cache_getfile (const cache_key_t &k, ptr<file_t> f)
  {
    //
    // Make a mapping of filenames -> options, so that we know which
    // cache entries are associated with a given filename (in case it's
    // later removed from the cache.
    //
    ptr<bhash<opts_t> > *hp, h;
    if (!( hp = _opts_map[k.fn ()] )) {
      h = New refcounted<bhash<opts_t> > ();
      _opts_map.insert (k.fn (), h);
      hp = &h;
    }
    (*hp)->insert (k.opts ());
    
    OKDBG4(PUB3_CACHE, CHATTER, 
	   "cache INSERT: %s:%x\n", k.fn ().cstr (), k.opts ());
    
    _noent_cache.remove (k.fn ());
    _getfile_cache.insert (cached_getfile_t (k, f));
  }
  

  //-----------------------------------------------------------------------
  
  void crp_t::rm_file (str nm, opts_t opt)
  {
    cache_key_t k (nm, opt);
    _getfile_cache.remove (k);
  }

  //=======================================================================

};
