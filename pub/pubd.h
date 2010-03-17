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

#pragma once

#include "arpc.h"
#include "pub.h"
#include "okclone.h"
#include "timehash.h"
#include "pub3.h"
#include "list.h"

//=======================================================================

namespace pub3 {

  //--------------------------------------------------------------------

  struct srv_cache_key_t {

    srv_cache_key_t (ptr<fhash_t> h, u_int o) : 
      _filehash (h), _opts (op_mask (o)), _hshkey (hash_me ()) {}

    srv_cache_key_t (ptr<const file_t> f) :
      _filehash (f->metadata ()->hashp ()), 
      _opts (op_mask (f->opts ())), 
      _hshkey (hash_me ()) {}

    hash_t hash_me () const;
    
    static u_int op_mask (u_int in) { return in & P_NOPARSE; }
    operator hash_t () const { return _hshkey; }
    bool operator== (const srv_cache_key_t &k) const 
    { return _opts == k._opts && *_filehash == *k._filehash; }
    str to_str () const;
    
    ptr<const fhash_t> _filehash;
    u_int _opts;
    hash_t _hshkey;
  };

  //-----------------------------------------------------------------------

  struct srv_cached_getfile_t {
    srv_cached_getfile_t (ptr<file_t> f) : _key (f), _file (f) {} 
    srv_cache_key_t _key;
    ptr<file_t> _file;
  };

  //-----------------------------------------------------------------------

  struct srv_cached_badfile_t {
    srv_cached_badfile_t (const srv_cache_key_t &k, 
			  parse_status_t s, const str &m)
      : _key (k), _stat (s), _msg (m) {}
    srv_cache_key_t _key;
    parse_status_t _stat;
    str _msg;
  };
};

//=======================================================================

template<> struct keyfn<pub3::srv_cached_getfile_t, pub3::srv_cache_key_t> {
  keyfn () {}
  const pub3::srv_cache_key_t &operator() (const pub3::srv_cached_getfile_t *o)
    const { return o->_key; }
};

//=======================================================================

template<> struct keyfn<pub3::srv_cached_badfile_t, pub3::srv_cache_key_t> {
  keyfn () {}
  const pub3::srv_cache_key_t &operator() (const pub3::srv_cached_badfile_t *o)
    const { return o->_key; }
};

//=======================================================================

namespace pub3 {

  class slave_srv_t;

  //-----------------------------------------------------------------------

  class srv_file_lookup_t : public file_lookup_t {
  public:
    virtual ~srv_file_lookup_t () {}
    virtual bool do_pushes () const { return false; }
    virtual void register_client (slave_srv_t *c) {}
    virtual void unregister_client (slave_srv_t *c) {}
  };
  
  //-----------------------------------------------------------------------

  class master_srv_t;

  //-----------------------------------------------------------------------

  class slave_srv_t {
  public:
    slave_srv_t (ptr<axprt_stream> x, master_srv_t *m, bool do_srv);
    slave_srv_t ();
    void dispatch (svccb *sbp);
    void getfile (svccb *sbp, CLOSURE) ;
    void config (svccb *sbp) {}
    void get_fstats (svccb *sbp) {}
    void push_deltas (ptr<xpub3_delta_set_t> s, evb_t cb, CLOSURE);
    void getchunk (svccb *sbp);
    void handle_eof ();
    
    ptr<srv_file_lookup_t> file_lookup ();
    ptr<local_publisher_t> pub () ;
    ptr<const local_publisher_t> pub () const;

    list_entry<slave_srv_t> _lnk;
    
  protected:
    ptr<axprt_stream> _x;
    ptr<aclnt> _cli;
    ptr<asrv> _srv;
    time_t _last_update;
    bool _push_deltas;
    bool _registered;
    bool _push_deltas_lock;
    master_srv_t *_master;
  };

  //-----------------------------------------------------------------------

  class master_srv_t : public clone_server_t {
  public:
    master_srv_t (ptr<axprt_unix> x, ptr<local_publisher_t> lp, 
		  ptr<srv_file_lookup_t> fl, int fdfd);
    ~master_srv_t ();
    void register_newclient (ptr<axprt_stream> x);

    slave_srv_t *new_slave (ptr<axprt_stream> x, bool do_srv);
    void delete_slave (slave_srv_t *s);
    
    void dispatch (svccb *sbp);
    void handle_clonefd (svccb *cbp);
    void handle_eof ();
    bool do_chroot (str d, str uname, str gname);
    void run (evi_t ev, CLOSURE);
    str jail2real (str d) const;
    ptr<srv_file_lookup_t> file_lookup () { return _file_lookup; }
    ptr<local_publisher_t> pub () { return _pub; }
    ptr<const local_publisher_t> pub () const { return _pub; }
  protected:
    ptr<axprt_unix> _x;
    ptr<asrv> _srv;
    ptr<local_publisher_t> _pub;
    ptr<srv_file_lookup_t> _file_lookup;
    slave_srv_t *_personal_slave;
    evv_t::ptr _shutdown_ev;
  };
  
  //-----------------------------------------------------------------------
  // Caching Pub Server Implementation
  //
  
  class cached_lookup_obj_t {
  public:
    cached_lookup_obj_t (str j, str r, ptr<fhash_t> h, time_t m, off_t sz)
      : _jfn (j), _rfn (r), _hsh (h), _ctime (m), _size (sz) {}
    
    void to_xdr (xpub3_fstat_t *x) const;
    time_t ctime () const { return _ctime; }
    ptr<fhash_t> hash () const { return _hsh; }
    const str &real_fn () const { return _rfn; }
    const str &jailed_fn () const { return _jfn; }
    off_t size () const { return _size; }
    
    // hack around files modified twice in the same minute
    void inc_ctime () { _ctime ++; }
    
  private:
    str _jfn;  // jailed filed name
    str _rfn;  // real file name
    ptr<fhash_t> _hsh;
    time_t _ctime;
    off_t _size;
  };
};

//=======================================================================
  
template<> struct keyfn<pub3::cached_lookup_obj_t, str> {
  keyfn () {}
  const str &operator() (const pub3::cached_lookup_obj_t *o) const 
  { return o->jailed_fn (); }
};
  
//=======================================================================

namespace pub3 {
  class jailed_file_t {
  public:
    jailed_file_t (str j, str r) : _jfn (j), _rfn (r) {}
    const str &real_fn () const { return _rfn; }
    const str &jailed_fn () const { return _jfn;}
  private:
    str _jfn, _rfn;
  };
};
  
//=======================================================================

template<> struct keyfn<pub3::jailed_file_t, str> {
  keyfn () {}
  const str &operator() (const pub3::jailed_file_t *j) const
  { return j->jailed_fn (); }
};

//=======================================================================

namespace pub3 {

  //-----------------------------------------------------------------------

  typedef enum { STAMP_NOCHANGE = 0, 
		 STAMP_TIMEOUT = 1, 
		 STAMP_FNF = 2,
		 STAMP_ERROR = 3, 
		 STAMP_CHANGED = 4,
		 STAMP_DISABLED = 5,
		 STAMP_UNINIT = 6 } stamp_status_t;

  //-----------------------------------------------------------------------

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

  //-----------------------------------------------------------------------

  class chunkholder_t: public srv_file_lookup_t {
  public:
    chunkholder_t () :  
      srv_file_lookup_t (),
      _chunk_cache (ok_pub3_chunk_lease_time, true) {}

    int hold_chunks (ptr<file_t> p);
    ptr<file_t> get_chunks (ptr<fhash_t> h, opts_t opts);
    static ptr<chunkholder_t> alloc ();
  private:
    timehash_t<srv_cache_key_t, srv_cached_getfile_t> _chunk_cache;

  };
  
  //-----------------------------------------------------------------------

  class srv_cache_t : public srv_file_lookup_t {
  public:
    srv_cache_t () : 
      srv_file_lookup_t (), 
      _getfile_cache (ok_pub3_getfile_object_lifetime, true),
      _badfile_cache (ok_pub3_getfile_object_lifetime, true),
      _last_update (0), 
      _delta_id (0) {}
    
    virtual ~srv_cache_t () { if (_timer) *_timer = false; }
    
    bool do_pushes () const { return true; }
    
    bool lookup (str nm, ptr<fhash_t> *hsh, time_t *ctime) ;
    void cache_lookup (str j, str r, ptr<fhash_t> hsh, time_t ctime,
		       off_t sz) ;
    bool getfile (ptr<fhash_t> h, opts_t opts, ptr<file_t> *f, 
		  parse_status_t *s, str *em);
    void cache_getfile (ptr<fhash_t> h, opts_t opts, ptr<file_t> f, 
			parse_status_t s, str em);
    static ptr<srv_cache_t> alloc ();

    int hold_chunks (ptr<file_t> p) ;
    ptr<file_t> get_chunks (ptr<fhash_t> h, opts_t opts);

    void set_ts_files (const str &s, const str &h)
    {
      _tss.setfile (s);
      _tsh.setfile (h);
    }

    void expire_old_entries (time_t timeout);
    void start_timer (u_int n = 0, u_int x = 0, u_int i = 0, u_int nc = 0,
		      u_int t = 0);
    
    void register_client (slave_srv_t *c) { _list.insert_head (c); }
    void unregister_client (slave_srv_t *c) { _list.remove (c); }
    void add_delta (str nm) { _delta_set.insert (nm); }

  protected:
    virtual void refresh_delta_set (evb_t ev, CLOSURE);
    virtual void push_deltas (ptr<xpub3_delta_set_t> s, evb_t cb, CLOSURE);
    void trav_key (str k);
    void trav_nkey (str k);
    
    ptr<bool> _timer;
    
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
    void
    run_refresh_timer (ptr<bool> *out,
		       u_int ival1, u_int ival2, u_int eln, u_int tsi,
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
    timehash_t<srv_cache_key_t, srv_cached_getfile_t> _getfile_cache;
    timehash_t<srv_cache_key_t, srv_cached_badfile_t> _badfile_cache;
    list<slave_srv_t, &slave_srv_t::_lnk> _list;
    bhash<str> _delta_set;
    time_t _last_update;
    int64_t _delta_id;

    // for treestat, stampfiles for sentinel and for heartbeat.
    stampfile_t _tss, _tsh;
  };
};

//
//-----------------------------------------------------------------------
