// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_PUB2_H
#define _LIBPUB_PUB2_H 1

#include "pub.h"
#include "tame.h"
#include "timehash.h"

namespace pub2 {

  typedef xpub_status_t status_t;

  // callbacks used for our primitive functions.
  typedef callback<void, status_t>::ref status_cb_t;
  typedef callback<void, status_t, ptr<bound_pfile2_t> >::ref getfile_cb_t;

  
  /**
   * Abstract version of classes that dump Pub output, given
   * access to a simple lookup/getfile interface.
   */
  class abstract_publisher_t : public pub2_iface_t, public pub_config_iface_t {
  public:
    abstract_publisher_t (u_int o = 0) 
      : _opts (o), _base_cfg (), _genv (&_base_cfg) {}

    virtual ~abstract_publisher_t () {}

    /**
     * Clients can call this function to publish the given file to 
     * the given zbuf. Note that it returns a full error code,
     * in case the caller wanted to know more about the error that
     * happened.
     *
     * @param b The zbuf to output the file to
     * @param fn The jailed name of the file to publish
     * @param cb The callback to call once the operation has finished
     * @param a The associative array to resolve variables with
     * @param opt publishing options to use.
     * @param e The environment to evaluate it (the global one by def.)
     */
    void run_full (zbuf *b, pfnm_t fn, status_cb_t cb, 
		   aarr_t *a = NULL, u_int opt = 0, penv_t *e = NULL,
		   CLOSURE);

    /**
     * this is a simplified version of the above, with a simpler status
     * return message. See above for a description of the parameters.
     */
    void run (zbuf *b, pfnm_t fn, cbb cb, aarr_t *a = NULL,
	      u_int opt = 0, penv_t *e = NULL,
	      CLOSURE);

    // implement the pub2_iface_t contract; only call internally
    // from pub objects.
    void publish (output_t *o, pfnm_t fn, penv_t *env, int lineno,
		  status_cb_t coordvar) 
    { publish_T (o, fn, env, lineno, coordvar); }

    /**
     * Read in and process configuration variables from a config file.
     * Callback response is a full status code, and not just a bool.
     *
     * @param fn the name of the top-level config variable.
     * @param cb the callback to call after we're done publishing.
     * @param res the aarr_t to write the results out to; associates
     *            with default publishing environment if none given.
     */
    void run_cfg_full (pfnm_t nm, status_cb_t cb, aarr_t *dest = NULL, 
		       CLOSURE);

    void run_cfg (pfnm_t nm, cbb cb, aarr_t *dest = NULL, CLOSURE);


    /**
     * Clear the base layer of pub variable bindings (read in by run_cfg).
     */
    void cfg_clear () { _base_cfg.clear (); }

    penv_t *get_env () const { return &_genv; }

    u_int opts () const { return _opts; }
    void set_opts (u_int i) { _opts = i; }

    void list_files_to_check (const pfnm_t &n, vec<str> *out);

  protected:
    // to be filled in by the sub classes
    virtual void getfile (pfnm_t fn, getfile_cb_t cb, u_int o = 0) = 0;

    virtual void publish_T (output_t *o, pfnm_t fn, penv_t *env, int lineno,
			    status_cb_t cb, CLOSURE);
    
    virtual bool is_remote () const = 0;

    u_int _opts;
    aarr_t _base_cfg;
    mutable penv_t _genv;   // global eval env
    str _cwd;               // CWD for publishing files
  };

  /**
   * Keys for caching published files, both based on their filename,
   * and the desired publishing options; for instance, we need
   * different copies for WSS and non-WSS files.
   */
  struct cache_key_t {
    cache_key_t (pfnm_t n, u_int o) 
      : _fn (n), _opts (op_mask (o)), _hsh (hash_me ()) {}

    const str &fn () const { return _fn; }
    u_int opts () const { return _opts; }

    const str _fn;
    const u_int _opts;
    const hash_t _hsh;

    bool operator== (const cache_key_t &k) const
    { return _fn == k._fn && _opts == k._opts; }
    operator hash_t () const { return _hsh; }

  private:
    // mask in only those options that matter for indexing.
    static u_int op_mask (u_int in) { return in & (P_WSS | P_NOPARSE); }
    hash_t hash_me () const {
      strbuf b (_fn);
      b << ":" << _opts;
      str s = b;
      return hash_string (s.cstr ());
    }
  };

  /**
   * A getfile request that has been cached is stored here.
   */
  struct cached_getfile_t {
    cached_getfile_t (const cache_key_t &k, ptr<bound_pfile2_t> f)
      : _key (k), _file (f) {}

    const cache_key_t _key;
    ptr<bound_pfile2_t> file () const { return _file; }
    ptr<bound_pfile2_t> _file;
  };

  /**
   * A global lookup class for looking up cached pubfile
   * attributes.  See pub/pubd2.h for an example of a useful
   * global_t
   */
  class file_lookup_t {
  public:
    virtual ~file_lookup_t () {}

    virtual bool lookup (pfnm_t nm, phashp_t *hsh, time_t *ctime) 
    { return false; }
    virtual void cache_lookup (pfnm_t j, pfnm_t r, phashp_t hsh, time_t ctime,
			       off_t sz) {}
    virtual bool getfile (ptr<pbinding_t> b, u_int opts,
			  ptr<bound_pfile2_t> *f, pubstat_t *s,
			  str *em) { return false; }
    virtual void cache_getfile (ptr<pbinding_t> b, u_int opts,
				ptr<bound_pfile2_t> f, pubstat_t s,
				str em) {}
  };

  // should only need one of these, since it doesn't really do
  // anything!
  extern file_lookup_t g_file_nocache_lookup;

  class local_publisher_t : public abstract_publisher_t {
  public:
    local_publisher_t (pub_parser_t *p, u_int opts = 0,
		       file_lookup_t *l = &g_file_nocache_lookup)
      : abstract_publisher_t (opts), _parser (p), _lookup (l) 
    {}

    virtual ~local_publisher_t () {}

    // read a file off the disk, without any caching
    void getfile (pfnm_t fn, getfile_cb_t cb, u_int o = 0) 
    {
      xpub2_file_freshcheck_t frsh (XPUB2_FRESH_NONE);
      getfile (fn, cb, frsh, o);
    }

    void getfile (pfnm_t fn, getfile_cb_t cb,
		  const xpub2_file_freshcheck_t &fres, u_int o = 0);

  protected:
    bool is_remote () const { return false; }

    pub_parser_t *_parser;
    file_lookup_t *_lookup;
  };

  /**
   * A convenience class just for parsing configration files
   * during program initialization.
   */
  class configger_t : public local_publisher_t, public virtual refcount {
  public:
    static ptr<configger_t> alloc (u_int o = 0)
    {
      zinit ();
      ptr<pub_parser_t> p = New refcounted<pub_parser_t> ();
      return New refcounted<configger_t> (p, o);
    }
    void setprivs (const str &d, const str &u, const str &g)
    { _parser->setprivs (d, u, g); }
    str jail2real (const str &s) { return _parser->jail2real (s); }

    configger_t (ptr<pub_parser_t> p, u_int o = 0)
      : local_publisher_t (p, o), _parser (p) {}

  private:
    ptr<pub_parser_t> _parser;
  };

  /**
   * The most basic remote publisher.  Never caches anything.
   */
  class remote_publisher_t : public abstract_publisher_t {
  public:
    remote_publisher_t (ptr<axprt_stream> x, u_int o = 0)
      : abstract_publisher_t (o),
	_x (x),
	_cli (aclnt::alloc (x, pub_program_2)),
	_srv (asrv_delayed_eof::alloc 
	      (x, pub_program_2, 
	       wrap (this, &remote_publisher_t::dispatch))) {}

    virtual ~remote_publisher_t () {}

    virtual void connect (cbb cb, CLOSURE);

    virtual void getfile (pfnm_t n, getfile_cb_t cb, u_int o = 0)
    { getfile_T (n, cb, o); }

    void dispatch (svccb *sbp);
    virtual void lost_connection () {}
    virtual void handle_new_deltas (svccb *sbp);
    

  protected:
    void getfile_T (pfnm_t nm, getfile_cb_t cb, u_int o = 0, CLOSURE) ;
    void getfile_body (pfnm_t nm, const xpub2_getfile_res_t *res, 
		       getfile_cb_t cb, CLOSURE);
    bool is_remote () const { return true; }

    virtual bool prepare_getfile (const cache_key_t &k, 
				  xpub2_getfile_arg_t *arg, 
				  ptr<bound_pfile2_t> *f,
				  status_t *status)
    { 
      arg->filename = k.fn ();
      arg->options = k.opts();
      arg->fresh.set_mode (XPUB2_FRESH_NONE);
      return false;
    }

    virtual void cache_getfile (const cache_key_t &k, 
				ptr<bound_pfile2_t> file) {}
    virtual ptr<bound_pfile2_t> file_nochange (const cache_key_t &k) 
    { return NULL; }
    virtual void cache_noent (pfnm_t nm) {}

  protected:
    ptr<axprt_stream> _x;
    ptr<aclnt> _cli;
    ptr<asrv> _srv;
  };

};

template<> struct keyfn<pub2::cached_getfile_t, pub2::cache_key_t> {
  keyfn () {}
  const pub2::cache_key_t &operator() (const pub2::cached_getfile_t *o) 
    const { return o->_key; }
};

namespace pub2 {

  typedef timehash_t<str,str> negcache_t;
  typedef timehash_t<cache_key_t, cached_getfile_t> getfile_cache_t;

  /**
   * A more advanced remote publisher that caches what it can,
   * periodically flushing the cache by accepting status updates
   * from pubd2.
   */
  class caching_remote_publisher_t : public remote_publisher_t {
  public:
    caching_remote_publisher_t (ptr<axprt_stream> x, u_int o = 0)
      : remote_publisher_t (x, o), _connected (false),
	_delta_id (-1), _noent_cache (ok_pub2_svc_neg_cache_timeout) {}

    bool prepare_getfile (const cache_key_t &k, xpub2_getfile_arg_t *arg,
			  ptr<bound_pfile2_t> *f, status_t *status);
    void cache_getfile (const cache_key_t &k, ptr<bound_pfile2_t> file);
    void cache_noent (pfnm_t nm);

    void connect (cbb cb, CLOSURE);
    void dispatch (svccb *sbp);

    void lost_connection ();
    void handle_new_deltas (svccb *sbp);
  protected:
    void handle_new_deltas (const xpub2_delta_set_t &s);
    void clear_cache () { _getfile_cache.clear (); }
    void do_file (xpub2_fstat_t st, bool *keep_me, const u_int &opt);
    void rm_file (pfnm_t nm, const u_int &opt);

    bool _connected;
    int64_t _delta_id;

  private:
    getfile_cache_t _getfile_cache;
    negcache_t _noent_cache;
    qhash<str, ptr<bhash<u_int> > > _opts_map;
  };

  /**
   * An output class for outputting configuration files with
   * pub2.
   */
  class output_conf2_t : public output_t {
  public:
    output_conf2_t (aarr_t *a = NULL) 
      : output_t (PFILE_TYPE_CONF2), _dest (a) {}
    ~output_conf2_t () {}
    void output_set_func (penv_t *e, const pfile_set_func_t *s);
    void output_err (penv_t *e, const str &s, int l = -1);
    void set_dest (aarr_t *d) { _dest = d; }
  private:
    aarr_t *_dest;
  };


};


#endif /* _LIBPUB_PUB2_H */
