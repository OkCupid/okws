
// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Max
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _PUB_PUBD2_H
#define _PUB_PUBD2_H

#include "arpc.h"
#include "pub.h"
#include "xpub.h"
#include "okclone.h"
#include "timehash.h"
#include "xpub.h"
#include "pub2.h"

namespace pubserv2 {
  struct cache_key_t {
    cache_key_t (phashp_t h, u_int o) : 
      _filehash (h), _opts (op_mask (o)), _hshkey (hash_me ()) {}
    hash_t hash_me () const;
    
    static u_int op_mask (u_int in) { return in & ( P_WSS); }
    operator hash_t () const { return _hshkey; }
    bool operator== (const cache_key_t &k) const 
    { return _opts == k._opts && *_filehash == *k._filehash; }
    
    phashp_t _filehash;
    u_int _opts;
    hash_t _hshkey;
  };

  struct cached_getfile_t {
    cached_getfile_t (const cache_key_t &k, ptr<bound_pfile2_t> f)
      : _key (k), _file (f) {}
    cache_key_t _key;
    ptr<bound_pfile2_t> _file;
  };

  struct cached_badfile_t {
    cached_badfile_t (const cache_key_t &k, pubstat_t s, const str &m)
      : _key (k), _stat (s), _msg (m) {}
    cache_key_t _key;
    pubstat_t _stat;
    str _msg;
  };
};

template<> struct keyfn<pubserv2::cached_getfile_t, pubserv2::cache_key_t> {
  keyfn () {}
  const pubserv2::cache_key_t &operator() (const pubserv2::cached_getfile_t *o)
    const { return o->_key; }
};

template<> struct keyfn<pubserv2::cached_badfile_t, pubserv2::cache_key_t> {
  keyfn () {}
  const pubserv2::cache_key_t &operator() (const pubserv2::cached_badfile_t *o)
    const { return o->_key; }
};


namespace pubserv2 {

  class srv_t;

  class file_lookup_t : public pub2::file_lookup_t {
  public:
    virtual ~file_lookup_t () {}
    virtual bool do_pushes () const { return false; }
    virtual void register_client (srv_t *c) {}
    virtual void unregister_client (srv_t *c) {}
  };
  
  class srv_t : public pub2::local_publisher_t {
  public:
    srv_t (ptr<axprt_stream> x, file_lookup_t *l);
    virtual ~srv_t ();
    void dispatch (svccb *sbp);
    void getfile (svccb *sbp, CLOSURE) ;
    void config (svccb *sbp) {}
    void get_fstats (svccb *sbp) {}
    void push_deltas (ptr<xpub2_delta_set_t> s, cbb cb, CLOSURE);

    virtual void handle_clonefd (svccb *sbp);
    virtual void handle_eof ();
    
    list_entry<srv_t> _lnk;
    
  protected:
    ptr<axprt_stream> _x;
    ptr<aclnt> _cli;
    ptr<asrv> _srv;
    time_t _last_update;
    bool _push_deltas;
    bool _registered;
    bool _push_deltas_lock;
    file_lookup_t *_file_lookup;
  };

  class primary_srv_t : public clone_server_t, public srv_t {
  public:
    primary_srv_t (ptr<axprt_stream> x, file_lookup_t *l, int fdfd);
    void register_newclient (ptr<axprt_stream> x);
    void handle_clonefd (svccb *cbp);
    void handle_eof ();
  };
  
  //-----------------------------------------------------------------------
  // Caching Pub Server Implementation
  //
  
  class cached_lookup_obj_t {
  public:
    cached_lookup_obj_t (pfnm_t j, pfnm_t r, phashp_t h, time_t m, off_t sz)
      : _jfn (j), _rfn (r), _hsh (h), _ctime (m), _size (sz) {}
    
    void to_xdr (xpub2_fstat_t *x)
    {
      x->fn = _jfn;
      x->ctime = _ctime;
      _hsh->to_xdr (&x->hash);
    }
    time_t ctime () const { return _ctime; }
    phashp_t hash () const { return _hsh; }
    const pfnm_t &real_fn () const { return _rfn; }
    const pfnm_t &jailed_fn () const { return _jfn; }
    off_t size () const { return _size; }
    
    // hack around files modified twice in the same minute
    void inc_ctime () { _ctime ++; }
    
  private:
    str _jfn;  // jailed filed name
    str _rfn;  // real file name
    phashp_t _hsh;
    time_t _ctime;
    off_t _size;
  };
};

  
template<> struct keyfn<pubserv2::cached_lookup_obj_t, str> {
  keyfn () {}
  const str &operator() (const pubserv2::cached_lookup_obj_t *o) const 
  { return o->jailed_fn (); }
};
  
namespace pubserv2 {
  class jailed_file_t {
  public:
    jailed_file_t (pfnm_t j, pfnm_t r) : _jfn (j), _rfn (r) {}
    const pfnm_t &real_fn () const { return _rfn; }
    const pfnm_t &jailed_fn () const { return _jfn;}
  private:
    str _jfn, _rfn;
  };
};
  
template<> struct keyfn<pubserv2::jailed_file_t, str> {
  keyfn () {}
  const str &operator() (const pubserv2::jailed_file_t *j) const
  { return j->jailed_fn (); }
};


namespace pubserv2 {

  typedef enum { STAMP_NOCHANGE = 0, 
		 STAMP_TIMEOUT = 1, 
		 STAMP_FNF = 2,
		 STAMP_ERROR = 3, 
		 STAMP_CHANGED = 4,
		 STAMP_DISABLED = 5,
		 STAMP_UNINIT = 6 } stamp_status_t;

  class stampfile_t {
  public:
    stampfile_t () : _last_ctime (0), _last_change_local (0) {}
    stampfile_t (const str &f) 
      : _fn (f), _last_ctime (0), _last_change_local (0) {}

    /**
     * @brief stats a stampfile, calling stat(2) and comparing to storted
     *    ctime for the file.
     * @param timeout return STAMP_TIMEOUT if timeout is > 0 and
     *   file hasn't changed in the timeout interval.
     * @return a status code from enumerate type
     */
    stamp_status_t stat (int timeout = 0);

    void setfile (const str &f) { _fn = f; }

    const str &fn () const { return _fn; }

  private:
    str _fn;                      // filename of the stamp file
    time_t _last_ctime;           // last ctime, as reported by stat(2)
    time_t _last_change_local;    // local time of the last change
  };
  
  class cache_t : public file_lookup_t {
  public:
    cache_t () : file_lookup_t (), 
		 _getfile_cache (ok_pub2_getfile_object_lifetime, true),
		 _badfile_cache (ok_pub2_getfile_object_lifetime, true),
		 _last_update (0), _delta_id (0) {}
    
    virtual ~cache_t () { if (_timer) _timer->cancel (); }
    
    bool do_pushes () const { return true; }
    
    bool lookup (pfnm_t nm, phashp_t *hsh, time_t *ctime) ;
    void cache_lookup (pfnm_t j, pfnm_t r, phashp_t hsh, time_t ctime,
		       off_t sz) ;
    bool getfile (ptr<pbinding_t> b, u_int opts,
		  ptr<bound_pfile2_t> *f, pubstat_t *s,
		  str *em);
    void cache_getfile (ptr<pbinding_t> b, u_int opts,
			ptr<bound_pfile2_t> f, pubstat_t s,
			str em);

    void set_ts_files (const str &s, const str &h)
    {
      _tss.setfile (s);
      _tsh.setfile (h);
    }

    void expire_old_entries (time_t timeout);
    void start_timer (u_int n = 0, u_int x = 0, u_int i = 0, u_int nc = 0,
		      u_int t = 0);
    
    void register_client (srv_t *c) { _list.insert_head (c); }
    void unregister_client (srv_t *c) { _list.remove (c); }
    void add_delta (pfnm_t nm) { _delta_set.insert (nm); }

  protected:
    virtual void refresh_delta_set (cbb cb, CLOSURE);
    virtual void push_deltas (ptr<xpub2_delta_set_t> s, cbb cb, CLOSURE);
    void trav_key (str k);
    void trav_nkey (str k);
    
    ptr<canceller_t> _timer, _nc_timer;
    
    /**
     * Start the refresh timer; usually it fires every ival1 seconds,
     * but if it senses that there's editting going on (files changing
     * ctimes), then it will refresh every ival2 seconds. If elen
     * interval pass without an edit, then we fall back to old interval.
     *
     * Note, we check the treestat files ever tsi (treestat interval)
     * seconds, regardless of the other timers.
     *
     * Returns a canceller object to cancel this timer.
     */
    ptr<canceller_t> 
    run_refresh_timer (u_int ival1, u_int ival2, u_int eln, u_int tsi,
		       CLOSURE);

    /**
     * Check the sentinel files to perhaps force an update, or 
     * perhaps skip an update
     */
    stamp_status_t check_sentinel (int timeout);
    
  protected:
    
  private:
    timehash_t<str, jailed_file_t> _noent_cache;
    timehash_t<str, cached_lookup_obj_t> _lookup_cache;
    timehash_t<cache_key_t, cached_getfile_t> _getfile_cache;
    timehash_t<cache_key_t, cached_badfile_t> _badfile_cache;
    list<srv_t, &srv_t::_lnk> _list;
    bhash<str> _delta_set;
    time_t _last_update;
    int64_t _delta_id;

    // for treestat, stampfiles for sentinel and for heartbeat.
    stampfile_t _tss, _tsh;

  };
};

//
//-----------------------------------------------------------------------

#endif /* _PUB_PUBD2_H */
