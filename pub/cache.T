// -*-c++-*-
/* $Id$ */

#include "pubd.h"
#include "okdbg.h"
#include "parseopt.h"
#include "pub3.h"

namespace pub3 {

  //=======================================================================

  bool
  srv_cache_t::lookup (str nm, ptr<fhash_t> *hsh, time_t *ctime)
  {
    cached_lookup_obj_t *o;
    if ((o = _lookup_cache[nm])) {
      *hsh = o->hash ();
      *ctime = o->ctime ();
      return true;
    } else if (_noent_cache.rejuvenate (nm)) {
      *hsh = NULL;
      return true;
    } else {
      return false;
    }
  }
  
  //-----------------------------------------------------------------------

  void
  srv_cache_t::cache_lookup (str jfn, str rfn, ptr<fhash_t> hsh, 
			     time_t ctime, off_t sz)
  {
    cached_lookup_obj_t *op = _lookup_cache[jfn];
    if (hsh) {
      if (!op || ctime > op->ctime () || sz != op->size () ||
	  *hsh != *op->hash ()) {
	_lookup_cache.insert (cached_lookup_obj_t (jfn, rfn, hsh, ctime, sz));
	
	// op is true => modification; op false => file being added
	// for first time, and therefore shouldn't be flushed from
	// services.
	if (op)
	  add_delta (jfn);
      }
    } else {
      
      if (!_noent_cache[jfn]) {
	_noent_cache.insert (jailed_file_t (jfn, rfn));
      }
      if (op) {
	_lookup_cache.remove (jfn);
	add_delta (jfn);
      }
    }
  }
  
  //-----------------------------------------------------------------------

  void
  srv_cache_t::expire_old_entries (time_t timeout)
  {
    time_t l = okwstime () - timeout;
    _noent_cache.expire (l);
    _lookup_cache.expire (l);
  }
  
  //-----------------------------------------------------------------------

  void
  srv_cache_t::trav_key (str k)
  {
    struct stat sb;
    cached_lookup_obj_t *o = _lookup_cache[k];
    assert (o);
    str rfn = o->real_fn ();

    if (nfs_safe_stat (rfn.cstr (), &sb) < 0) {

      // a file that used to exist has gone missing
      _lookup_cache.remove (k);

      // After removing k, we cannot access o, since o is now pointiing
      // to deallocated data after the above call to remove.
      o = NULL;

      _noent_cache.insert (jailed_file_t (k, rfn));
      add_delta (k);

    } else {
      
      bool dirty = false;
      if (o->ctime () < time_t (sb.st_ctime) || o->size () != sb.st_size) {
	dirty = true;
      } 
      // could have been modified twice in the same second;
      // hack around this by bumping the ctime artificially up on the
      // cached version of the file.
      else if (o->ctime () == sb.st_ctime && okwstime () > sb.st_ctime) {
	ptr<fhash_t> h = file2hash (rfn);
	if (!h || *h != *o->hash ()) {
	  dirty = true;
	  
	  // break the tie if possible, so we don't have to keep hashing the
	  // file over and over
	} else 
	  o->inc_ctime ();
      }
      
      if (dirty) {
	OKDBG4(PUB3_CACHE, CHATTER, "Cached file modified: %s\n", k.cstr ());

	// a file has be modified!
	_lookup_cache.remove (k);

	// As above, we can remove the k object from the list, but after
	// we do so, we better not access o again.
	o = NULL;

	add_delta (k);
      } 
    }  
  }

  //-----------------------------------------------------------------------
  
  void
  srv_cache_t::trav_nkey (str k)
  {
    struct stat sb;
    jailed_file_t *o = _noent_cache[k];
    assert (o);
    const str &rfn = o->real_fn ();
    if (nfs_safe_stat (rfn.cstr (), &sb) >= 0) {
      // a file that used to not exist has now come to exist
      _noent_cache.remove (k);
    }
    
    /*
     * Add the following code back if we decide it's a good idea to ship
     * the negative file lookup cache over to the service.  As of 2/4/06,
     * I think this is a bad idea, since it could invite DOS'ing the channel
     * between pub2 and the services.
     *edit = true;
     } else {
     s->misses.push_back (k);
     }
    */
  }
  
  //-----------------------------------------------------------------------
  
  static void fill_delta_set (ptr<xpub3_delta_set_t> d, const str &s)
  {
    d->files.push_back (s);
  }
  
  //-----------------------------------------------------------------------
  
  tamed void 
  srv_cache_t::refresh_delta_set (evb_t ev)
  {
    tvars { 
      bool edit (false);
      bool dummy;
      ptr<xpub3_delta_set_t> s;
    }
    
    OKDBG3 (PUB3_CACHE, CHATTER, "Timer fired; calling refresh_delta_set()");
    
    _lookup_cache.traverse_keys (wrap (this, &srv_cache_t::trav_key));
    _noent_cache.traverse_keys (wrap (this, &srv_cache_t::trav_nkey));
    
    if (_delta_set.size ()) {
      s = New refcounted<xpub3_delta_set_t> ();
      _delta_set.traverse (wrap (fill_delta_set, s));
      s->start = _last_update;
      s->stop = okwstime ();
      s->serial = _delta_id ++;
      _delta_set.clear ();
      _last_update = okwstime ();
      edit = true;
    }
    
    ev->trigger (edit);
    if (edit)
      twait { push_deltas (s, mkevent (dummy)); }
  }

  //-----------------------------------------------------------------------
  
  tamed void 
  slave_srv_t::push_deltas (ptr<xpub3_delta_set_t> s, evb_t ev)
  {
    tvars {
      clnt_stat err;
      bool rc (true);
    }
    if (!_push_deltas) {
      rc = true;
    } else if (_push_deltas_lock) {
      rc = false; 
    } else {
      _push_deltas_lock = true;
      twait { 
	rpc::pub_prog_3::pub3_push_deltas (_cli, s, &rc, mkevent (err)); 
      }
      _push_deltas_lock = false;
      if (err) {
	strbuf b;
	b << "RPC error pushing stats to service: " << err << "\n";
	okdbg_warn (ERROR, b);
	rc = false;
      } else if (!rc) {
	okdbg_warn (ERROR, "Service unexepectedly rejected stats push!\n");
      }
    }
    ev->trigger (rc);
  }

  //-----------------------------------------------------------------------
  
  tamed void 
  srv_cache_t::push_deltas (ptr<xpub3_delta_set_t> s, evb_t ev)
  {
    tvars { 
      rendezvous_t<ptr<bool> > rv (__FL__);
      ptr<bool> b;
      bool rc (true);
    }
    for (slave_srv_t *e = _list.first; e; e = _list.next (e)) {
      b = New refcounted<bool> ();
      e->push_deltas (s, mkevent (rv, b, *b));
    }
    while (rv.need_wait ()) {
      twait (rv, b); 
      if (!*b) 
	rc = false; 
    }
    ev->trigger (rc);
  }

  //------------------------------------------------------------------------

  stamp_status_t
  stampfile_t::stat (int timeout)
  {
    stamp_status_t ret = STAMP_ERROR;
    str stmp;
    time_t ct (0);

    if (!_fn) {
      ret = STAMP_DISABLED;
    } 

    // Stamp file should contain the ASCII timestamp of the last
    // update; best not to trust ctimes over NFS.
    //
    // XXX or could use nfs_safe_stat
    else if (!((stmp = file2str (_fn)) && convertint (stmp, &ct))) {
      ret = STAMP_FNF;
    } else if (ct == _last_ctime) {
      if (timeout > 0 && okwstime () - _last_change_local >= timeout) 
	ret = _last_change_local ? STAMP_TIMEOUT : STAMP_UNINIT;
      else
	ret = STAMP_NOCHANGE;
    } else {
      _last_ctime = ct;
      _last_change_local = okwstime ();
      ret = STAMP_CHANGED;
    }
    return ret;
  }
  
  //------------------------------------------------------------------------

  //
  // check the sentinel file and the heartbeat message.  if the sentinel
  // file has changed since the last run, then we need to update our
  // local ctime cache, so return 1.  If no update, then return 0.
  // If the heartbeat hasn't fired within the given wait period,
  // return -1 and do a regular check.  Or, if the interval is negative
  // or 0, return -1, signalling a regular check is needed.
  //
  // Returns:
  //  a stamp_status_t explaining what happened.
  // 
  stamp_status_t
  srv_cache_t::check_sentinel (int timeout)
  {
    stamp_status_t hbt = _tsh.stat (timeout);
    if (!(hbt == STAMP_CHANGED || hbt == STAMP_NOCHANGE)) {

      if (_tsh.fn () && hbt != STAMP_UNINIT)
	OKDBG4 (PUB3_CACHE, ERROR,
		"heartbeat file (%s) failed us, with status: %d",
		_tsh.fn ().cstr (), int (hbt));

      return hbt;
    }

    return _tss.stat ();
  }

  //------------------------------------------------------------------------

  /**
   * @brief loop for running the refresh timer for refreshing the cache.
   * @param n the minimum allowable freshness
   * @param x maximum freshness allowable
   * @param i move tolerance up by i second increments
   * @param t if > 0, t gives the interval for checking the tree heartbeats
   */
  tamed void
  srv_cache_t::run_refresh_timer (ptr<bool> *out, 
			      u_int n, u_int x, u_int i, u_int t)
  {
    tvars {
      bool run_loop (true);
      bool edit (false);
      int tol (n); // tolerance for how fresh
      time_t last_check (0);
      stamp_status_t status;
      bool running_refresh (false);
      int diff;
      ptr<bool> ok (New refcounted<bool> (true));
    }
    *out = ok;

    assert (t <= n);

    while (run_loop && *ok) {

      // Always update the freshness tolerance first, since it 
      // might immediately affect whether we sleep or launch
      // a new refresh_delta_set operation.
      if (running_refresh) {
	int old = tol;
	if (edit) {
	  tol = n;
	} else {
	  tol += i;
	  if (tol > int (x)) tol = x;
	}
	if (old != tol) 
	  OKDBG4(PUB3_CACHE,CHATTER, 
		 "Tolerance at time level %d reset to %d", old, tol);

	running_refresh = false;
      }

      status = t > 0 ? check_sentinel (tol) : STAMP_DISABLED;


      // run the refresh algorithm if the sentinel explicitly tells us
      // to, or we haven't checked in a while and we haven't be
      // explicitly told that there wasn't a change! However,
      // never wait longer than the max time, regardless of 
      // what then sentinel system says.
      diff = okwstime () - last_check;
      if (status == STAMP_CHANGED || 
	  (status != STAMP_NOCHANGE && diff >= tol) ||
	  diff > int (x)) {

	if (status == STAMP_CHANGED) 
	  OKDBG3(PUB3_CACHE, CHATTER, "stamp file changed; refresh forced");

	last_check = okwstime ();
	running_refresh = true;
	twait { refresh_delta_set (mkevent (edit)); }

      } else {
	twait { delaycb (t > 0 ? t : tol, 0, mkevent ()); }
      }
    }
  }

  //------------------------------------------------------------------------

  void
  srv_cache_t::start_timer (u_int n, u_int x, u_int i, u_int c, u_int t)
  {
    if (!n) n = ok_pub3_refresh_min;
    if (!x) x = ok_pub3_refresh_max;
    if (!i) i = ok_pub3_refresh_incr;
    if (!c) c = ok_pub3_neg_cache_timeout;
    if (!t) t = ok_pub3_treestat_interval;

    if (t > n) {
      okdbg_warn (ERROR,
		  "Cannot have treestat check interval greater than "
		  "minimum check interval!\n"
		  "Disabling treestat checking.\n");
      t = 0;
    }

    if (_timer) {
      *_timer = false;
    }
    run_refresh_timer (&_timer, n, x, i, t);
    _noent_cache.set_timeout (c);
  }
  
  //------------------------------------------------------------------------
  
  bool 
  srv_cache_t::getfile (ptr<fhash_t> h, opts_t opts,
			ptr<file_t> *f, parse_status_t *s,
			str *em)
  {
    bool ret (false);
    srv_cache_key_t k (h, opts);

    OKDBG3(PUB3_CACHE, CHATTER, strbuf ("getting file: ") << k.to_str ());

    srv_cached_getfile_t *cgf = _getfile_cache[k];
    if (cgf) {
      if (f) *f = cgf->_file;
      ret = true;
    } else {
      srv_cached_badfile_t *bf = _badfile_cache[k];
      if (bf) {
	if (s) *s = bf->_stat;
	if (em) *em = bf->_msg;
	ret = true;
      }
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void 
  srv_cache_t::cache_getfile (ptr<fhash_t> h, opts_t opts,
			      ptr<file_t> f, parse_status_t s,
			      str em) 
  {
    srv_cache_key_t k (h, opts);
    OKDBG3(PUB3_CACHE, CHATTER, strbuf ("caching file: ") <<  k.to_str ());
    if (f) {
      _badfile_cache.remove (k);
      _getfile_cache.insert (srv_cached_getfile_t (f));
    } else if (s != PARSE_OK) {
      _getfile_cache.remove (k);
      _badfile_cache.insert (srv_cached_badfile_t (k, s, em));
    } else {
      panic ("got a file neither good nor bad!\n");
    }
  }
  
  //-----------------------------------------------------------------------

  hash_t 
  srv_cache_key_t::hash_me () const
  {
    struct {
      char buf[PUBHASHSIZE];
      int opts;
    } tmp;

    memset (&tmp, 0, sizeof (tmp));
    memcpy (tmp.buf, _filehash->buf (), PUBHASHSIZE);
    tmp.opts = _opts;
    return hash_bytes (reinterpret_cast<const void *> (&tmp), sizeof (tmp));
  }

  //-----------------------------------------------------------------------

  str
  srv_cache_key_t::to_str () const
  {
    strbuf b;
    b << _filehash->to_str () << ":" << _opts << " (" << _hshkey << ")";
    return b;
  }

  //-----------------------------------------------------------------------

  int
  srv_cache_t::hold_chunks (ptr<file_t> f) 
  {
    return _getfile_cache.timeout ();
  }
  
  //-----------------------------------------------------------------------

  ptr<file_t>
  srv_cache_t::get_chunks (ptr<fhash_t> h, opts_t opts)
  {
    ptr<file_t> ret;
    getfile (h, opts, &ret, NULL, NULL);
    return ret;
  }

  //-----------------------------------------------------------------------

  ptr<srv_cache_t> srv_cache_t::alloc () 
  { return New refcounted<srv_cache_t> (); }

  //=======================================================================

  int
  chunkholder_t::hold_chunks (ptr<file_t> f)
  {
    srv_cached_getfile_t cf (f);
    _chunk_cache.insert (cf);

    if (OKDBG2(PUB3_CACHE)) {
      xpub3_hash_t hsh;
      xpub3_file_t xf;
      strbuf b ("hold_chunks(key = { hsh : ");
      cf._key._filehash->to_xdr (&hsh);
      rpc_print (b, hsh);
      str fn = cf._file->metadata ()->jailed_filename ();
      b.fmt (", opts : %du }) (%s)", cf._key._opts, fn.cstr ());
      okdbg_warn (CHATTER,b);
    }

    return _chunk_cache.timeout ();
  }

  //-----------------------------------------------------------------------

  ptr<file_t> 
  chunkholder_t::get_chunks (ptr<fhash_t> h, opts_t opts)
  {
    srv_cache_key_t k (h, opts);
    srv_cached_getfile_t *cfg = _chunk_cache[k];
    return cfg ? cfg->_file : NULL;
  }

  //-----------------------------------------------------------------------

  void 
  cached_lookup_obj_t::to_xdr (xpub3_fstat_t *x) const
  {
    x->fn = _jfn;
    x->ctime = _ctime;
    _hsh->to_xdr (&x->hash);
  }

  //-----------------------------------------------------------------------

  ptr<chunkholder_t> chunkholder_t::alloc () 
  { return New refcounted<chunkholder_t> (); }

  //=======================================================================
};
