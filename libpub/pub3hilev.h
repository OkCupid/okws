// -*-c++-*-

#pragma once

#include "pub3eval.h"
#include "pub3file.h"
#include "pub3ast.h"
#include "pub3expr.h"
#include "pub3eval.h"
#include "pub3cache.h"
#include "pub3parse.h"

namespace pub3 {

  typedef xpub_status_t status_t;
  typedef event<status_t, ptr<file_t> >::ref getfile_ev_t;

  //=======================================================================

  class localizer_t : public virtual refcount {
  public:
    localizer_t () {}
    virtual ~localizer_t () {}
    virtual str localize (const str &infn) const;
    virtual bool localize_many (const str &infn, vec<str> *out) const;
    virtual void compound_localize (const str &infn, vec<str> *out) const;
    virtual bool strict () const { return false; }
    virtual str get_default_fnf () const { return NULL; }
  };

  //======================================================================
  //
  // Hi-level interface for all okpublishers, copied for the most part
  // from pub v2.  There are a few changes here --- first, we're passing
  // in expr_dict_t's rather than aarrs.
  //

  class ok_iface_t : public virtual refcount {
  public:
    virtual ~ok_iface_t () {}

    /**
     * Run the publisher on file fn, writing the output to buffer b.
     *
     * @param b the buffer to output to
     * @param fn the filename to publish
     * @param ev the event to trigger after complete; true for success.
     * @param opts flags to affect run behavior
     * @param sp Status pointer -- return the full pub status if the 
     *      caller specifies a non-null slot.
     * @param fp File pointer --- return the pubbed file pointer if the
     *      caller specified a non-null slot.
     */
    virtual void run (zbuf *b, str fn, evb_t ev, 
		      ptr<expr_dict_t> d = NULL, 
		      opts_t opts = -1, 
		      status_t *sp = NULL, 
		      ptr<file_t> *fp = NULL, CLOSURE) = 0;

    /**
     * Run the publisher, ignoring output, just amassing all universals
     * and globals into the given dictionary.]
     *
     * @param fn the file to publisher
     * @param ev the event to trigger on completion; true for success
     * @param d the dinctionary to publish to. if none given, assume universals.
     * @param sp status pointer -- return the full pub status if the
     *      caller specified a non-null slot.
     *
     */  
    virtual void run_cfg (str fn, evb_t ev, 
			  ptr<expr_dict_t> d = NULL,
			  opts_t opts = -1,
			  status_t *sp = NULL, CLOSURE) = 0;

    /**
     * syntax check the file
     *
     * @param f the file to syntax check
     * @param err report the error here (if the caller specified non-null)
     * @param ev trigger this event with 0 for success and -1 for error
     */
    virtual void syntax_check (str f, vec<str> *err, evi_t ev, CLOSURE) = 0;

    /**
     * Publish the given file
     * @param p the publishing state
     * @param fn the file to publish
     * @param ev the event to trigger with the status
     */
    virtual void publish (eval_t *p, str fn, getfile_ev_t ev, CLOSURE) = 0;

    /**
     * fetch the file and metadata from the disk
     */
    virtual void publish_prepare (eval_t *p, str fn, str *rfn, str *errp,
				   getfile_ev_t ev, CLOSURE) = 0;

    // set/get global ops for this publishing interface.
    virtual opts_t opts () const = 0;
    virtual void set_opts (opts_t i) = 0;

    static ptr<expr_dict_t> get_universals ();
    static pub3::obj_t get_universals_obj ();
    static pub3::obj_dict_t pub3_config_obj ();
  };

  //=======================================================================
  //
  // The abstract publisher implements most of the above functions,
  // but depends on a subclass to implement getfile().
  //
  class abstract_publisher_t : public ok_iface_t {
  public:
    abstract_publisher_t (opts_t o = 0, ptr<const localizer_t> l = NULL);

    void run (zbuf *b, str fn, evb_t ev, ptr<expr_dict_t> d = NULL, 
	      opts_t opts = 0, status_t *sp = NULL, ptr<file_t> *fp = NULL,
	      CLOSURE);

    void run_pub (eval_t *p, str fn, evb_t ev,
		  status_t *sp = NULL, ptr<file_t> *fp = NULL,
		  CLOSURE);

    void run_cfg (str fn, evb_t ev, ptr<expr_dict_t> d = NULL,
		  opts_t opts = -1, status_t *sp = NULL, CLOSURE);

    opts_t opts () const { return _opts; }
    void set_opts (opts_t o) { _opts = o; }
    void syntax_check (str f, vec<str> *errors, evi_t ev, CLOSURE);
    void set_err_obj_key (str s) { _pub3_err_obj_key = s; }

    void init_for_run (eval_t *p, opts_t o, ptr<expr_dict_t> d);
    void uninit_for_run (eval_t *p);
    void publish (eval_t *p, str fn, getfile_ev_t ev, CLOSURE);
    ptr<const localizer_t> get_localizer (eval_t *p);
    void set_localizer (ptr<const localizer_t> l) { _localizer = l; }
    void publish_prepare (eval_t *p, str fn, str *rfn, str *errp,
			  getfile_ev_t ev, CLOSURE);

  protected:
    // to be filled in by the sub classes
    virtual void getfile (str fn, getfile_ev_t ev, opts_t o = 0) = 0;
    virtual bool is_remote () const = 0;

    void list_files_to_check (str cwd, str n, vec<str> *out, 
			      ptr<const localizer_t> l);
  private:
    opts_t _opts;
    str _pub3_err_obj_key;
    ptr<const localizer_t> _localizer;
  };

  //=======================================================================

  /**
   * The most basic remote publisher.  Never caches anything.
   */
  class remote_publisher_t : public abstract_publisher_t {
  public:
    remote_publisher_t (ptr<axprt_stream> x, opts_t o = 0);
    virtual ~remote_publisher_t () {}

    virtual void connect (evb_t cb, CLOSURE);

    void getfile (str fn, getfile_ev_t cb, opts_t o = 0);

    void dispatch (svccb *sbp);
    virtual void lost_connection () {}
    virtual void handle_new_deltas (svccb *sbp);
    

  protected:

    //------------------------------------------------------------
    // All involved with getting a file, and dealing with its
    // chunks, if necessary.
    //
    void getfile_body (str nm, const xpub3_getfile_res_t *res, 
		       getfile_ev_t cb, opts_t opt, CLOSURE);

    void getfile_chunked (const xpub3_chunkshdr_t &hdr, opts_t opts,
			  xpub3_file_t *file, status_ev_t cb, CLOSURE);

    void getchunk (const xpub3_hash_t &key, opts_t opts,
		   size_t offset, size_t sz, char *buf, evb_t ok, CLOSURE);
    //
    //-----------------------------------------------------------------------


    bool is_remote () const { return true; }

    virtual bool prepare_getfile (const cache_key_t &k, 
				  xpub3_getfile_arg_t *arg, 
				  ptr<file_t> *f,
				  status_t *status);

    virtual void cache_getfile (const cache_key_t &k, ptr<file_t> file) {}
    virtual ptr<file_t> file_nochange (const cache_key_t &k) { return NULL; }
    virtual void cache_noent (str nm) {}
    void getfile_T (str fn, getfile_ev_t cb, opts_t o, CLOSURE);

  protected:
    ptr<axprt_stream> _x;
    ptr<aclnt> _cli;
    ptr<asrv> _srv;
    size_t _maxsz;
  };

  //=======================================================================

  class local_publisher_t : public abstract_publisher_t {
  public:
    local_publisher_t (ptr<pub_parser_t> p = NULL, 
		       opts_t opts = 0,
		       ptr<file_lookup_t> fl = NULL,
		       ptr<jailer_t> jl = NULL);

    virtual ~local_publisher_t () {}

    // read a file off the disk, without any caching
    void getfile (str fn, getfile_ev_t cb, opts_t o = 0);
    void getfile (str fn, getfile_ev_t cb,
		  const xpub3_file_freshcheck_t &fres, opts_t o = 0);
    void set_jailer (ptr<jailer_t> j) { _jailer = j; }
    ptr<jailer_t> jailer () { return _jailer; }
    ptr<const jailer_t> jailer () const { return _jailer; }
    
  protected:
    bool is_remote () const { return false; }
    ptr<pub_parser_t> _parser;
    ptr<file_lookup_t> _lookup;
    ptr<jailer_t> _jailer;
  };

  //=======================================================================
  
  /**
   * A more advanced remote publisher that caches what it can,
   * periodically flushing the cache by accepting status updates
   * from pubd2.
   */
  class caching_remote_publisher_t : public remote_publisher_t {
  public:
    caching_remote_publisher_t (ptr<axprt_stream> x, opts_t o = 0)
      : remote_publisher_t (x, o), 
	_connected (false),
	_delta_id (-1), 
	_noent_cache (ok_pub3_svc_neg_cache_timeout) {}

    bool prepare_getfile (const cache_key_t &k, xpub3_getfile_arg_t *arg,
			  ptr<file_t> *f, status_t *status);
    void cache_getfile (const cache_key_t &k, ptr<file_t> file);
    void cache_noent (str nm);

    void connect (evb_t cb, CLOSURE);
    void dispatch (svccb *sbp);

    void lost_connection ();
    void handle_new_deltas (svccb *sbp);
    bool is_cached (str n, opts_t o, const fhash_t &hsh) const;
  protected:
    void handle_new_deltas (const xpub3_delta_set_t &s);
    void clear_cache () { _getfile_cache.clear (); }
    void do_file (xpub3_fstat_t st, bool *keep_me, opts_t opt);
    void rm_file (str nm, opts_t opt);
    bool invalidate_cache (str nm);

    bool _connected;
    int64_t _delta_id;

  private:
    getfile_cache_t _getfile_cache;
    negcache_t _noent_cache;
    qhash<str, ptr<bhash<opts_t> > > _opts_map;
  };

  //=======================================================================
};
