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

#ifndef _LIBPUB_PUB_H
#define _LIBPUB_PUB_H 1

#include "okws_sfs.h"
#include "vec.h"
#include "qhash.h"
#include "clist.h"
#include "pubutil.h"
#include "arpc.h"
#include "puberr.h"
#include "holdtab.h"
#include "zstr.h"
#include "tame.h"
#include "pscalar.h"
#include "parseopt.h"

#if defined (SFSLITE_PATCHLEVEL) && SFSLITE_PATCHLEVEL >= 10000000
# include "tame_lock.h"
# define TAME_LOCKING 1
#elif defined(SFSLITE_PATCHLEVEL) && SFSLITE_PATCHLEVEL >= 8009003
# include "lock.h"
# define TAME_LOCKING 1
#endif /* SFSLITE_PATCHLEVEL */

extern int yywss;          /* on if in White-space strip mode, off otherwise */

struct yy_buffer_state;
class pstr_t;
class pstr_el_t;
class penv_t;

typedef enum { PFILE_HTML_EL = 0, PFILE_INC = 1, 
	       PFILE_SWITCH = 2, PFILE_VAR = 3, 
	       PFILE_PSTR = 4, PFILE_CCODE = 5, 
	       PFILE_GFRAME = 6, PFILE_FILE = 7, 
	       PFILE_SEC = 8, PFILE_INCLUDE = 9,
	       PFILE_FUNC = 10, PFILE_INCLIST = 11,
	       PFILE_INCLUDE2 = 12, PFILE_RAW = 13 } pfile_el_type_t;

typedef enum { PFILE_TYPE_NONE = 0,
	       PFILE_TYPE_GUY = 1,
	       PFILE_TYPE_H = 2,
	       PFILE_TYPE_WH = 3,
	       PFILE_TYPE_CODE = 4,
	       PFILE_TYPE_EC = 5,
	       PFILE_TYPE_WEC = 6,
               PFILE_TYPE_CONF = 7,
	       PFILE_TYPE_CONF2 = 8} pfile_type_t;

typedef enum { PUBSTAT_OK = 0, 
	       PUBSTAT_FNF = 1, 
	       PUBSTAT_PARSE_ERROR = 3,
	       PUBSTAT_READ_ERROR = 4 } pubstat_t;

xpub_status_typ_t pub_stat2status (pubstat_t in);

typedef enum { EXPLORE_PARSE = 0, EXPLORE_FNCALL = 1,
	       EXPLORE_CONF = 2 } pub_exploremode_t;

typedef enum { EVAL_FULL = 0, EVAL_INTERNAL = 1, 
	       EVAL_COMPILE = 2, EVAL_SIMPLE = 3 } pub_evalmode_t;

typedef enum { JAIL_NONE = 0, JAIL_VIRTUAL = 1, 
	       JAIL_REAL = 2, JAIL_PERMISSIVE = 3 } jail_mode_t;

typedef enum { PARR_OK = 0, PARR_BAD_TYPE = 1, PARR_OUT_OF_BOUNDS = 2,
	       PARR_OVERFLOW = 3, PARR_NOT_FOUND = 4 } parr_err_t;


/* include options */
#define P_COMPILE 1 << 0        /* no eval errors tolerated */
#define P_DEBUG   1 << 1        /* debug info output w/ text */
#define P_IINFO   1 << 2        /* include info output w/ text */
#define P_VERBOSE 1 << 3        /* debug messages, etc */
#define P_DAEMON  1 << 4        /* running as background daemon */
#define P_VISERR  1 << 5        /* visible HTML errors */
#define P_WSS     1 << 6        /* white space stripping */
#define P_CLEAR   1 << 7        /* used to clear all other options */

#define P_EXPLORE_OFF (1 << 8)   // don't explore files after parse
#define P_INCLUDE_V2  (1 << 9)   // support V2 include semantics
#define P_EXPORTER    (1 << 10)  // export files via RPC
#define P_NOPARSE     (1 << 11)  // don't parse file at all
#define P_NOLOCALE    (1 << 12)  // Don't localize file
#define P_GLOBALSET   (1 << 13)  // Allow non-local sets
#define P_CONFIG      (1 << 14)  // Treat all files as configs

/* XXX - defaults should be put someplace better */
#define P_INFINITY   65334

/* shortcut macros for common operations on the global pub object */
#define PWARN(x)   parser->pwarn (strbuf () << x)
#define PFILE      parser->bpf->file
#define PARSEFAIL  PFILE->err = PUBSTAT_PARSE_ERROR;
#define PLINC      PFILE->inc_lineno ()
#define PLINENO    (PFILE->get_lineno ())
#define PSECTION   PFILE->section
#define PFUNC      parser->top_func ()
#define ARGLIST    PFUNC->arglist
#define PLASTHTAG  parser->lasttag
#define PHTAG      parser->tag
#define PAARR      parser->aarr
#define PSTR1      parser->str1
#define PPSTR      parser->pstr
#define PARR       parser->parr
#define ARGLIST2   parser->arglist

#define PUSH_PFUNC(x) do { parser->push_func (x); } while (0)
#define POP_PFUNC()   parser->pop_func()

#define PUB_NULL_CASE "NULL"

//
// static xdr <--> zstr functions
//
void zstr_to_xdr (const zstr &z, xpub_zstr_t *x, 
		  int l = Z_BEST_COMPRESSION);
zstr xdr_to_zstr (const xpub_zstr_t &x);

#define MAXLEV 1024
#define DUMP_INDENT 5
class dumper_t {
public:
  dumper_t () : level (0) {}
  void begin_obj (const str &typ, void *p, int l);
  void end_obj ();
  void output (const str &s, bool nl = true);
private:
  void set_sp_buf ();
  char sp_buf[MAXLEV];
  int level;
};

class dumpable_t {
public:
  dumpable_t () {}
  virtual ~dumpable_t () {}
  virtual void dump (dumper_t *d) const;
  virtual void dump2 (dumper_t *d) const {}
  virtual str get_obj_name () const = 0;
  virtual int get_lineno () const { return 0; }
};

#define DUMP(d,s)  (d)->output (strbuf () << s)

class concatable_t {
public:
  virtual ~concatable_t () {}
  virtual bool concat (const str &s) { return false; }
  virtual bool concat (char c) { return false ; }
  virtual bool concat (concatable_t *c) { return false; }
  virtual str to_str () const { return NULL; }
};

class concatable_str_t : public virtual concatable_t {
public:
  concatable_str_t () {}
  concatable_str_t (const str &s) { concat (s); }
  concatable_str_t (char c) { concat (c); }
  concatable_str_t (const zstr &zz) : z (zz) {}
  ~concatable_str_t () {}
  bool concat (const str &s); 
  // bool concat (strbuf *s) { sb << *s; return true; }
  bool concat (char c) { sb.tosuio ()->copy (&c, 1);  return true; }
  bool concat (concatable_t *l);
  str to_str () const { return sb; }
  const zstr &to_zstr () const { if (z) return z; else return (z = sb); }
private:
  strbuf sb;
  mutable zstr z;
  vec<str> hold;
};

typedef bhash<str> gvtab_t;
class gvars_t : public virtual dumpable_t {
public:
  gvars_t () {}
  virtual ~gvars_t () {}
  void add (const str &s) { tab.insert (s); }
  virtual str get_obj_name () const { return "gvars_t"; }
  void dump2 (dumper_t *d) const;
  virtual int lookup (const str &s) const { return (tab[s] ? 1 : 0); }
protected:
  gvtab_t tab;
};

class guvars_t : public gvars_t {
public:
  int lookup (const str &s) const { return (tab[s] ? -1 : 0); }
  str get_obj_name () const { return "guvars_t"; }
};

class aarr_t;
class aarr_arg_t;
class pval_t;
class pfile_t;
class bound_pfile_t;
class output_t;
typedef ptr<const bound_pfile_t> bpfcp_t; // const pointer
typedef ptr<bound_pfile_t> bpfmp_t;       // mutable pointer

//-----------------------------------------------------------------------

class nvpair_t : public virtual dumpable_t {
public:
  nvpair_t (const str &n, ptr<pval_t> v) : nm (n), val (v) {}
  nvpair_t (const xpub_nvpair_t &x);
  nvpair_t (const nvpair_t &p) : nm (p.nm), val (p.val) {}
  virtual ~nvpair_t () {}
  const str &name () const { return nm; }
  const pval_t *value () const { return val; }
  pval_t *value () { return val; }
  void output (output_t *o, penv_t *e, int i = 0) const;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "nvpair_t"; }
  bool to_xdr (xpub_nvpair_t *x) const;

  void set_value (ptr<pval_t> r) { val = r; }

  ptr<pval_t> value_ptr () { return val; }
  ptr<const pval_t> value_ptr () const { return val; }

  const str nm;
  ihash_entry<nvpair_t> hlink;
private:
  ref<pval_t> val;
};

//-----------------------------------------------------------------------

class nvtab_t : 
  public ihash<const str, nvpair_t, &nvpair_t::nm, &nvpair_t::hlink>
{

private:
  typedef ihash<const str, nvpair_t, &nvpair_t::nm, &nvpair_t::hlink> super_t;

public:

  void copy (const nvtab_t &dest);
  void overwrite_with (const nvtab_t &dest);
  void insert (nvpair_t *p);
};

class pval_w_t {
public:
  pval_w_t () : val (NULL), int_err (INT_MIN), env (NULL), ival_flag (false),
		ival (0) {}
  pval_w_t (pval_t *v, penv_t *e) 
    : val (v), int_err (INT_MIN), env (e), ival_flag (false), ival (0) {}
  pval_w_t (const str &n, penv_t *e)
    : val (NULL), name (n), int_err (INT_MIN), env (e), ival_flag (false),
      ival (0){}
  pval_w_t (int i) : 
    val (NULL), int_err (INT_MIN), env (NULL), ival_flag (true), ival (i) {}

  inline operator int() const { return to_int (); }
  inline operator str() const { return to_str () ; }
  pval_w_t operator[] (size_t i) const { return elem_at (i); }

  str to_str () const;
  int to_int () const;
  pval_w_t elem_at (size_t i) const;
  size_t size () const;
private:
  const pval_t *get_pval () const;
  pval_t *val;
  const str name;
  const int int_err;
  penv_t *env;
  
  const bool ival_flag;
  const int ival;
};

// a generic interface for classes that support the basic PUB output
// method
class outputable_t {
public:
  virtual ~outputable_t () {}
  virtual void output (output_t *o, penv_t *e) const = 0;
};

//-----------------------------------------------------------------------

class aarr_t : public virtual dumpable_t, public virtual outputable_t {
public:
  aarr_t () {}
  aarr_t (const xpub_aarr_t &x);
  aarr_t (const aarr_t &in) { aar.copy (in.aar); }
  virtual ~aarr_t () { aar.deleteall (); }
  void add (nvpair_t *p);
  aarr_t &add (const str &n, const str &v);
  aarr_t &add (const str &n, zbuf *b);
  aarr_t &add (const str &n, ptr<zbuf> zb);
  aarr_t &add (const str &n, const strbuf &b) { return add (n, str (b)); }
  void remove (nvpair_t *p);

  aarr_t &replace (const str &n, ptr<pval_t> v);
  ptr<pval_t> &value_ref (const str &n);
  const ptr<pval_t> &value_ref (const str &n) const;
  template<class T> aarr_t &replace_so (const str &n, T i);

  aarr_t &replace (const str &n, int64_t i) { return replace_so (n, i); }
  aarr_t &replace (const str &n, double d) { return replace_so (n, d); }
  aarr_t &replace (const str &n, const str &s) { return replace_so (n, s); }

  template<class T> aarr_t &add (const str &n, T i);

  aarr_t &operator= (const aarr_t &in);
  aarr_t &overwrite_with (const aarr_t &r);
  pval_t *lookup (const str &n);
  const pval_t *lookup (const str &n) const;

  ptr<pval_t> lookup_ptr (const str &n);
  ptr<const pval_t> lookup_ptr (const str &n) const;

  void output (output_t *o, penv_t *e) const;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "aarr_t"; }
  bool to_xdr (xpub_aarr_t *x) const;
  const nvtab_t *nvtab () const { return &aar; }
  nvtab_t *nvtab () { return &aar; }
  void deleteall () { aar.deleteall (); }
  size_t size () const { return aar.size (); }

protected:
  nvtab_t aar;
private:
  friend class penv_t;
  clist_entry<aarr_t> slnk;
};

//-----------------------------------------------------------------------

struct penv_state_t {
  penv_state_t (u_int o, size_t e, bool f) 
    : opts (o), estack_size (e), errflag (f) {}
  u_int opts;
  size_t estack_size;
  bool errflag;
};

template<class K>
class bhash_copyable_t : public bhash<K>
{
public:
  bhash_copyable_t () : bhash<K>() {}
  bhash_copyable_t (const bhash_copyable_t &h)
  {
    const qhash_slot<K,void> *s;
    for (s = h.first (); s; s = h.next (s))  {
      insert (s->key);
    }
  }
};

class pub_localizer_t : public virtual refcount {
public:
  pub_localizer_t () {}
  virtual ~pub_localizer_t () {}
  virtual str localize (const str &infn) const = 0;
};

class penv_t {
public:
  penv_t (aarr_t *a = NULL, u_int o = 0, aarr_t *g = NULL);
  penv_t (const penv_t &e);

  ~penv_t () {}

  penv_state_t *start_output (aarr_t *a, u_int o);
  bool finish_output (penv_state_t *s);

  void resize (size_t s);
  void gresize (size_t gvs);
  void resize (size_t s, size_t gvs) { resize (s); gresize (gvs); }
  size_t size () const { return estack.size (); }
  size_t gvsize () const { return gvars.size (); }
  void push (aarr_t *a) { estack.push_back (a); }
  void safe_push (ptr<const aarr_t> a);
  bool set_global (const aarr_t &a);
  void push (const gvars_t *g) { gvars.push_back (g); }
  const pval_t *lookup (const str &n, bool recurse = true);
  pval_w_t operator[] (const str &n) { return pval_w_t (n, this); }
  pub_evalmode_t init_eval (pub_evalmode_t m = EVAL_FULL);
  void eval_pop (const str &n);
  void set_evalmode (pub_evalmode_t m) { evm = m; }
  str loc (int l = -1) const;
  pfnm_t filename () const;
  bool output_info () const { return (opts & P_IINFO); }

  void push_file (bpfcp_t f);
  void pop_file ();
  
  bool debug () const { return getopt (P_DEBUG) && (evm != EVAL_INTERNAL); }
  u_int getopts () const { return opts; }
  void compile_err (const str &s);
  void warning (const str &s) const;
  bool success () const { return !cerrflag; }

  bool go_g_var (const str &n) const;
  bool is_gvar (const str &v) const;

  str stack_to_str () const;
  bool i_stack_add (bpfcp_t t);
  void i_stack_remove (bpfcp_t t);

  void setlineno (int i) { olineno = i; }
  void unsetlineno () { olineno = -1; }
  bool getopt (u_int opt) const { return (opts & opt); }
  u_int setopts (u_int no) { u_int oo = opts; opts = no; return oo; }

  bool set_tlf (bool b) { bool r = tlf; tlf = b; return r; }
  bool get_tlf () const { return tlf; }
  void clear () { estack.clear (); gvars.clear (); hold.clear (); }

  void set_localizer (ptr<const pub_localizer_t> l) { _localizer = l; }
  ptr<const pub_localizer_t> localizer () const { return _localizer; }

  int aarr_n;
  bpfcp_t file;
  bool needloc;
  str cerr;
  u_int opts;
private:

  pub_evalmode_t evm;
  qhash<str, vec<ssize_t> > evaltab;
  vec<const aarr_t *> estack; // eval stack
  vec<const gvars_t *> gvars;
  vec<bpfcp_t> fstack;
  vec<ptr<const aarr_t> > hold;
  bhash_copyable_t<phashp_t> istack;
  int olineno;
  bool cerrflag; // compile error flag
  bool tlf; // top level flag
#ifdef TAME_LOCKING
  lock_t _lock;
#endif /* TAME_LOCKING */
  ptr<const pub_localizer_t> _localizer;

  ptr<aarr_t> _global_set;
};

class pfile_set_func_t;
class output_t {
public:
  output_t (pfile_type_t m, u_int o = 0) 
    : mode (m), _opts (o), _muzzled (false) {}

  virtual ~output_t () {}

  virtual void output_raw (penv_t *e, const str &s) {}
  virtual void output (penv_t *e, const zstr &s, bool quoted = true) {}
  virtual void output (penv_t *e, zbuf *zb, bool quoted = true) {}
  virtual void output_err (penv_t *e, const str &s, int l = -1) = 0;
  virtual void output_info (penv_t *e, const str &s, int l = -1) {}
  virtual void output_header (penv_t *e) {}
  virtual void output_err_stacktrace (penv_t *e, const str &s, int l = -1);

  virtual void output_file_loc (penv_t *e, int lineno) {}
  virtual void output_set_func (penv_t *e, const pfile_set_func_t *s) = 0;

  virtual pfile_type_t switch_mode (pfile_type_t m) { return PFILE_TYPE_NONE; }
  virtual str switch_osink (const str &ns) { return NULL; }
  virtual str switch_class (const str &nc) { return NULL; }
  virtual str get_class () const { return NULL; }
  virtual bool descend () const { return true; }
  virtual bool stack_restore () const { return true; }
  virtual u_int opts () const { return _opts; }
  virtual bool wss () const { return _opts & P_WSS; }

  virtual bool set_muzzle (bool n);

  pfile_type_t mode;
  u_int _opts;
  bool _muzzled;
};

class output_std_t : public output_t {
public:
  output_std_t (zbuf *o, pfile_type_t m = PFILE_TYPE_H, u_int opt = 0) 
    : output_t (m, opt), out (o), osink_open (false) {}
  output_std_t (zbuf *o, const pfile_t *f = NULL);
  virtual ~output_std_t () {}

  void output_raw (penv_t *e, const str &s);
  void output (penv_t *e, zbuf *zb, bool quoted = true);
  void output (penv_t *e, const zstr &s, bool quoted = true);
  void output_err (penv_t *e, const str &s, int l = -1);
  void output_info (penv_t *e, const str &s, int l = -1);
  void output_header (penv_t *e);
  void output_file_loc (penv_t *e, int lineno);
  virtual void output_set_func (penv_t *e, const pfile_set_func_t *a);
  pfile_type_t switch_mode (pfile_type_t m);
  str switch_osink (const str &ns);
  str switch_class (const str &nc);
  str get_class () const { return classname; }

private:
  zbuf *out;
  str osink;
  str classname;
  bool osink_open;
};

class output_conf_t : public output_t {
public:
  output_conf_t (bool d = true) ;
  ~output_conf_t () {}
  void output_set_func (penv_t *e, const pfile_set_func_t *s);
  void output_err (penv_t *e, const str &s, int l = -1);
  bool descend () const { return dflag; }
  bool stack_restore () const { return false; }
  ptr<aarr_t> env;
private:
  bool dflag;
};


//-----------------------------------------------------------------------
// Pub2 Additions - most code for pub2 is in pub2.h, but the minimal
// amount of glue code is in pub.h

typedef callback<void, xpub_status_t>::ref xpub_status_cb_t;

// Pub2 Inteface for legacy objects
class pub2_iface_t {
public:
  virtual ~pub2_iface_t () {}
  virtual void publish (output_t *o, pfnm_t fn, penv_t *env, int lineno,
			xpub_status_cb_t cb) = 0;
};

// objects in the object tree need to inherit from this object to
// make the publishable in pub2.  Eventually we want to change these
// virtual methods to be abstract.
class pub2able_t : public virtual outputable_t {
public:
  
  // Try the nonblocking mode of publishing first, to save the
  // overhead of making and using a callback for simple document
  // elements.
  virtual bool publish_nonblock (pub2_iface_t *iface, output_t *o, 
				 penv_t *env) const 
  { output (o, env); return true; }
  
  // If that fails, the call the blocking version of publish.
  // Note that to simplify, we're putting a CLOSURE into the
  // virtual method signature.
  virtual void publish (pub2_iface_t *iface, output_t *o, penv_t *env,
			xpub_status_cb_t cb, CLOSURE) const
  { (*cb) (XPUB_STATUS_NOT_IMPLEMENTED); }
};

// For those classes that need to access the network, simply
// dump this macro in
#define PUBLISH_BLOCKS()                                      \
   bool publish_nonblock (pub2_iface_t *iface, output_t *o,   \
			  penv_t *env) const                  \
   { return false; }



// end pub2 additional classes
//-----------------------------------------------------------------------


struct bound_pfile_t : public virtual refcount, 
		       public virtual dumpable_t,
		       public virtual pub2able_t
{
  bound_pfile_t (const pbinding_t *b, pfile_t *f = NULL, 
		 const pfnm_t &r = NULL, bool del = false) 
    : bnd (b), file (f), rfn (r), delfile (del) {}
  bound_pfile_t (pfile_t *f = NULL) : bnd (NULL), file (f), delfile (false) {}
  ~bound_pfile_t ();

  static bpfmp_t alloc (const pbinding_t *b, pfile_t *f = NULL, 
			const pfnm_t &r = NULL, bool del = false)
  { return New refcounted<bound_pfile_t> (b, f, r, del); }
  
  static bpfmp_t alloc (pfile_t *f = NULL)
  { return New refcounted<bound_pfile_t> (f); }

  str get_obj_name () const { return "bound_pfile_t"; }
  void dump2 (dumper_t *d) const;
  bool open ();
  str read () const;
  void close ();

  // for pub2
  void publish (pub2_iface_t *iface, output_t *o, penv_t *genv,
		xpub_status_cb_t cb, CLOSURE) const;
  PUBLISH_BLOCKS ();

  str loc (int i = -1) const;
  phashp_t hash () const { return bnd->hash (); }
  pfnm_t filename () const { return bnd->filename (); }
  operator bool() const { return (bnd && file); }
  void output (output_t *o, penv_t *e) const;
  void explore (pub_exploremode_t m) const; 
  const pbinding_t *const bnd;
  pfile_t *const file;
  const pfnm_t rfn; // used only when publishing 
  bool delfile;

};

//-----------------------------------------------------------------------

class pbuf_t;
class evalable_t {
public:
  virtual ~evalable_t () {}
  str eval (penv_t *e, pub_evalmode_t m = EVAL_FULL, 
	    bool allownull = false) const;

  virtual bool eval_to_scalar (penv_t *e, scalar_obj_t *so) const;

  str eval () const { return eval (NULL, EVAL_SIMPLE); }
  ptr<pbuf_t> eval_to_pbuf (penv_t *e, pub_evalmode_t m) const;

  /**
   * eval_obj -- used internally in the eval mechanism.  Takes
   * as input this object, and evaluates it to the given pbuf_t.
   * The transformation is that all variables of the form ${VAR}
   * are resolved into an element suitable for output. The output
   * is not a regular string but a pbuf_t, since it will keep
   * gzip'ed information (AKA zbuf's) intact. However, the output
   * pbuf_t is guaranteed not to have any unresolved ${VAR}s
   * after the eval completes.
   */
  virtual void eval_obj (pbuf_t *s, penv_t *e, u_int d) const = 0;
  virtual str eval_simple () const { return NULL; }
};

//-----------------------------------------------------------------------

template<class T, clist_entry<T> T::*field>
struct publist_t 
  : public clist_t<T, field>
{
  void output (output_t *o, penv_t *en) const
  {
    for (T *e = clist_t<T,field>::first; e; e = next (e)) e->output (o, en); 
  }
  void dump (dumper_t *d) const
  {
    for (T *e = clist_t<T,field>::first; e; e = next (e)) e->dump (d);
  }

  // blocking publish interface
  void publish (pub2_iface_t *iface, output_t *o, penv_t *env,
		xpub_status_cb_t cb, CLOSURE) const;

  // if we're sure all elements in the list are nonblock elements, then
  // we can use this, but let's be careful here, so we make aggressive
  // assertions.
  void publish_nonblock (pub2_iface_t *iface, output_t *o, penv_t *env) const
  {
    for (T *e = clist_t<T,field>::first; e; e = next (e)) 
      assert (e->publish_nonblock (iface, o, env));
  }

};

typedef qhash<str, u_int> qhsi_t;

class pfile_frame_t : public virtual dumpable_t {
public:
  pfile_frame_t () : sss (0), sgvss (0) {}
  void mark_frame (penv_t *p) const 
  { sss = p->size () ; sgvss = p->gvsize (); }
  void push_frame (penv_t *p, aarr_t *f, const gvars_t *g = NULL) const;
  void pop_frame (output_t *o, penv_t *p) const 
  { if (o->stack_restore ()) p->resize (sss, sgvss); }
  virtual str get_obj_name () const { return "pfile_frame_t"; }
private:
  mutable int sss;    /* start stack size */
  mutable int sgvss;  /* start gvar stack size */
};

class parr_t;
class parr_mixed_t;
class parr_ival_t;
class nested_env_t;
class pub_regex_t;
class pub_range_t;
class pub_scalar_t;
class pub_aarr_t;

//-----------------------------------------------------------------------

class arg_t : public virtual refcount, public virtual dumpable_t,
	      public virtual evalable_t
{
public:
  virtual ~arg_t () {}
  virtual ptr<aarr_arg_t> to_aarr () { return NULL; }
  virtual ptr<const aarr_arg_t> to_aarr () const { return NULL; }
  virtual ptr<pub_aarr_t> to_pub_aarr () { return NULL; }
  virtual ptr<const pub_aarr_t> to_pub_aarr () const { return NULL; }
  virtual bool is_null () const { return false; }
  virtual ptr<pub_regex_t> to_regex () { return NULL; }
  virtual ptr<pub_range_t> to_range () { return NULL; }
  virtual ptr<pval_t> to_pval () { return NULL; }
  virtual bool to_int64 (int64_t *i) const { return false; }
  virtual ptr<const parr_mixed_t> to_mixed_arr () const { return NULL; }
  virtual ptr<parr_mixed_t> to_mixed_arr () { return NULL; }
  virtual const parr_ival_t *to_int_arr () const { return NULL; }
  virtual const parr_t *to_arr () const { return NULL; }
  virtual ptr<nested_env_t> to_nested_env () { return NULL; }
  virtual ptr<pub_scalar_t> to_scalar () { return NULL; }
  virtual ptr<const pub_scalar_t> to_scalar () const { return NULL; }
};

//-----------------------------------------------------------------------

class pval_t : public arg_t {
public:
  virtual ~pval_t () {}
  static ptr<pval_t> alloc (const xpub_val_t &x);
  ptr<pval_t> to_pval () { return mkref (this); }
  virtual bool to_xdr (xpub_val_t *x) const { return false; }
  virtual ptr<pval_t> flatten(penv_t *e) ;
  virtual ptr<pstr_t> to_pstr () { return NULL; }
};

//-----------------------------------------------------------------------

class pub_regex_t : public pval_t {
public:
  pub_regex_t (const str &r, const str &o) : _rxx_str (r), _opts (o) {}
  pub_regex_t (const xpub_regex_t &x);
  bool to_xdr (xpub_regex_t *x) const;
  bool compile (str *err);
  bool match (const str &s);
  ptr<pub_regex_t> to_regex () { return mkref (this); }
  str to_str () const { return _rxx_str; }
  str get_obj_name () const { return "regex"; }
  void eval_obj (pbuf_t *ps, penv_t *e, u_int d) const;

private:
  const str _rxx_str;
  const str _opts;
  ptr<rxx> _rxx;
};

//-----------------------------------------------------------------------

class pub_range_t : public pval_t {
public:
  virtual bool match (scalar_obj_t so) = 0;
  virtual bool to_xdr (xpub_range_t *r) = 0;
  virtual str to_str () const = 0;
  static ptr<pub_range_t> alloc (const str &pat, const str &o, str *e);
  static ptr<pub_range_t> alloc (const xpub_range_t &xpr);
  void eval_obj (pbuf_t *ps, penv_t *e, u_int d) const;
  ptr<pub_range_t> to_range () { return mkref (this); }
};

//-----------------------------------------------------------------------

class pub_irange_t : public pub_range_t {
public:
  pub_irange_t (int64_t l, int64_t h) : _low (l), _hi (h) {}
  pub_irange_t (const xpub_irange_t &x);
  str get_obj_name () const { return "irange"; }

  str to_str () const;
  bool match (scalar_obj_t so);
  bool to_xdr (xpub_range_t *r);

private:
  const int64_t _low, _hi;
};

//-----------------------------------------------------------------------

class pub_urange_t : public pub_range_t {
public:
  pub_urange_t (u_int64_t l, u_int64_t h) : _low (l), _hi (h) {}
  pub_urange_t (const xpub_urange_t &x);
  str get_obj_name () const { return "urange"; }

  str to_str () const;
  bool match (scalar_obj_t so);
  bool to_xdr (xpub_range_t *r);
private:
  const u_int64_t _low, _hi;
};

//-----------------------------------------------------------------------

class pub_drange_t : public pub_range_t {
public:
  pub_drange_t (double l, double h) : _low (l), _hi (h) {}
  pub_drange_t (const xpub_drange_t &x);
  str get_obj_name () const { return "irange"; }

  str to_str () const;
  bool match (scalar_obj_t so);
  bool to_xdr (xpub_range_t *r);
private:
  const double _low, _hi;
};

//-----------------------------------------------------------------------

class pub_aarr_t : public pval_t {
public:
  pub_aarr_t (ptr<aarr_t> a) : _obj (a) {}
  ptr<const aarr_t> obj () const { return _obj; }
  ptr<aarr_t> obj () { return _obj; }
  str get_obj_name () const { return "pub_aarr"; }
  ptr<const pub_aarr_t> to_pub_aarr () const { return mkref (this); }
  ptr<pub_aarr_t> to_pub_aarr () { return mkref (this); }
  static ptr<pub_aarr_t> alloc (ptr<aarr_t> a) 
  { return New refcounted<pub_aarr_t> (a); }
  void eval_obj (pbuf_t *ps, penv_t *e, u_int d) const {}

private:
  ptr<aarr_t> _obj;
};

//-----------------------------------------------------------------------

class pfile_el_t : public virtual concatable_t, public virtual dumpable_t,
		   public virtual pub2able_t {
public:
  virtual void output (output_t *o, penv_t *e) const = 0;
  virtual pfile_el_type_t get_type () const = 0;
  /* for list-based elements, turn them into individual elements */
  virtual bool sec_traverse_init () { return false; }
  virtual pfile_el_t *sec_traverse () { return NULL; }
  bool same_type_as (pfile_el_t *b) const 
  { return (get_type () == b->get_type ()); }
  clist_entry<pfile_el_t> lnk;
  virtual bool to_xdr (xpub_obj_t *x) const { return false; }
  static pfile_el_t *alloc (const xpub_obj_t &x);
  virtual void explore (pub_exploremode_t mode) const {}
};
typedef publist_t<pfile_el_t, &pfile_el_t::lnk> pfile_el_lst_t;

class pstr_el_t 
  : public virtual concatable_t, public virtual dumpable_t,
    public virtual evalable_t, public virtual pub2able_t {
public:
  static pstr_el_t * alloc (const xpub_pstr_el_t &x);
  virtual void output (output_t *o, penv_t *e) const = 0;
  virtual pfile_el_t *to_pfile_el () { return NULL; }
  clist_entry<pstr_el_t> lnk;
  virtual bool to_xdr (xpub_pstr_el_t *x) const { return false; }
};

class pvar_t;
class gcode_t;
class pstr_var_t;

//-----------------------------------------------------------------------

class pub_scalar_t : public pval_t {
public:
  pub_scalar_t (const str &n) : _obj (n) {}
  pub_scalar_t (int64_t i) { _obj.set (i); }
  pub_scalar_t (const scalar_obj_t &o) : _obj (o) {}
  pub_scalar_t () {}

  template<class T>
  static ptr<pub_scalar_t> alloc (T i)
  { return New refcounted<pub_scalar_t> (i); }

  ptr<pub_scalar_t> to_scalar () { return mkref (this); }
  ptr<const pub_scalar_t> to_scalar () const { return mkref (this); }
  const scalar_obj_t &obj () const { return _obj; }
  scalar_obj_t &obj () { return _obj; }
  void eval_obj (pbuf_t *s, penv_t *e, u_int d) const;
  str get_obj_name () const { return "pub_scalar_t"; }

  bool eval_to_scalar (penv_t *e, scalar_obj_t *o) const;
private:
  scalar_obj_t _obj;
};

//-----------------------------------------------------------------------

class pstr_t : public pval_t, public pub2able_t {
public:
  pstr_t (ptr<pvar_t> v);
  pstr_t (const str &s);
  pstr_t (const xpub_pstr_t &x);
  pstr_t () : n (0)  {}
  ~pstr_t () { els.deleteall(); }
  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pstr_t"; }
  void eval_obj (pbuf_t *ps, penv_t *e, u_int d) const;
  void add (const str &s);
  void add (char c);
  void add (pstr_el_t *v);
  void add (ptr<pvar_t> v);
  void output (output_t *o, penv_t *e) const;
  str eval_simple () const;
  pstr_el_t *shift ();
  bool to_xdr (xpub_pstr_t *x) const;
  bool to_xdr (xpub_val_t *x) const;
  ptr<pstr_t> to_pstr () { return mkref (this); }
  bool is_empty () const { return n == 0; }

protected:
  int n; // number of elements in the list (since it's a list)
  publist_t<pstr_el_t, &pstr_el_t::lnk> els;
};

class pval_zbuf_t : public pval_t {
public:
  // note no to_xdr function
  pval_zbuf_t (ptr<zbuf> z) : pval_t (), zb (z), zb_hold (z) {}
  pval_zbuf_t (zbuf *z) : pval_t (), zb (z) {}
  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "p_zbuf_val_t"; }
  void eval_obj (pbuf_t *ps, penv_t *e, u_int d) const;
  str eval_simple () const { return zb->output (); }
private:
  zbuf *zb;
  ptr<zbuf> zb_hold;
};

class pfile_sec_t : public pfile_el_t {
public:
  pfile_sec_t (int l, bool ls = true) 
    : els (ls ? New pfile_el_lst_t () : NULL), lineno (l),
      btag_flag (false), etag_flag (false) {}
  pfile_sec_t (const xpub_section_t &x);
  virtual ~pfile_sec_t () { if (els) { els->deleteall(); delete els; } }
  pfile_sec_t *add (pfile_el_t *el, bool combine = true);
  pfile_sec_t *add (pfile_sec_t *s);
  virtual pfile_sec_t *add (char c) { return this; }
  virtual pfile_sec_t *add (const str &s) { return this; }
  virtual void output (output_t *o, penv_t *e) const
  { if (els) els->output (o, e); }
  clist_entry<pfile_sec_t> lnk;
  virtual pfile_el_type_t get_type () const { return PFILE_SEC; }
  virtual bool is_empty () const { return false; }
  virtual bool to_xdr (xpub_section_t *s) const { return false; }

  virtual void dump2 (dumper_t *d) const { if (els) els->dump (d); }
  int get_lineno () const { return lineno; }
  virtual void explore (pub_exploremode_t mode) const;
  virtual bool is_tag () const { return false; }

  // functions for adding HTML fragments
  inline void hadd (pfile_sec_t *s);
  template<class C> inline void hadd (C c)
  {
    etag_flag = false;
    apply_space ();
    add (c);
  }

  // functions used for weeding out spaces from wss-html
  virtual inline void hadd_space ();
  inline void htag_space () 
  { if (!btag_flag) add (' '); else btag_flag = false;}
  inline void btag_set (bool b) { btag_flag = b; }
  inline void etag_reset () { etag_flag = true; }
  inline void apply_space ();

  // for pub2
  void publish (pub2_iface_t *, output_t *, penv_t *, xpub_status_cb_t ,
		CLOSURE) const;

  // if no elements in this section, can ignore it
  bool publish_nonblock  (pub2_iface_t *, output_t *, penv_t *) const
  { return (!els); }

  pfile_el_lst_t *els;
protected:
  int lineno;
  bool btag_flag;
  bool etag_flag;
};
typedef publist_t<pfile_sec_t, &pfile_sec_t::lnk> pfile_sec_lst_t;

class arg_t;
typedef vec<ptr<arg_t> > arglist_t;

class pfile_func_t : public pfile_frame_t, public pfile_el_t {
public:
  pfile_func_t (int l) : lineno (l) {}
  virtual bool add (ptr<arglist_t> al) = 0;
  virtual bool validate () { return false; }
  virtual void explore (pub_exploremode_t mode) const {}
  void include (output_t *o, penv_t *genv, aarr_t *lenv, const pfnm_t &nm) 
    const;
  virtual pfile_el_type_t get_type () const { return PFILE_FUNC; }

  // for pub2
  void publish (pub2_iface_t *iface, output_t *o, penv_t *genv,
		aarr_t *e, pfnm_t fn, xpub_status_cb_t cb, CLOSURE) const;

  int get_lineno () const { return lineno; }
  virtual bool muzzle_output () const { return false; }
  ptr<arglist_t> arglist;
protected:
  int lineno;
};

class pfile_inclist_t : public pfile_func_t {
public:
  pfile_inclist_t (int l) : pfile_func_t (l), err (false), ali (0) {}
  pfile_inclist_t (const xpub_inclist_t &x);
  ~pfile_inclist_t () {}
  void output (output_t *o, penv_t *e) const {}
  bool add (ptr<arglist_t> l);
  bool validate ();
  void explore (pub_exploremode_t mode) const;
  pfile_el_type_t get_type () const { return PFILE_INCLIST; }
  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pfile_inclist_t"; }
  bool to_xdr (xpub_obj_t *x) const;
protected:
  bool err;
  vec<pfnm_t> files;
  u_int ali; // arglist index
};

class pfile_include_t : public pfile_func_t {
public:
  pfile_include_t (int l, ptr<aarr_arg_t> e = NULL) : 
    pfile_func_t (l), err (false), env (e) {}
  pfile_include_t (const xpub_include_t &x);
  virtual ~pfile_include_t () {}
  virtual void output (output_t *o, penv_t *e) const;
  virtual bool add (ptr<arglist_t> l);
  virtual bool validate ();
  virtual void explore (pub_exploremode_t mode) const;
  virtual pfile_el_type_t get_type () const { return PFILE_INCLUDE; }

  virtual void dump2 (dumper_t *d) const;
  virtual str get_obj_name () const { return "pfile_include_t"; }
  virtual bool to_xdr (xpub_obj_t *x) const;
  void to_xdr_base (xpub_obj_t *x) const;
  bool add_base (ptr<arglist_t> l);

  // For pub2
  virtual void publish (pub2_iface_t *, output_t *, penv_t *, 
			xpub_status_cb_t , CLOSURE) const;
  virtual bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;
protected:
  bool err;
  pfnm_t fn;
  ptr<aarr_arg_t> env;
};

//-----------------------------------------------------------------------

class pfile_for_t : public pfile_func_t {
public:
  pfile_for_t (int l) : pfile_func_t (l) {}
  pfile_for_t (const xpub_for_t &x);
  bool to_xdr (xpub_obj_t *x) const;
  bool add (ptr<arglist_t> l);
  bool add_env (ptr<nested_env_t> e) { _env = e; return true; }
  str get_obj_name () const { return "pfile_for_t"; }
  virtual void publish (pub2_iface_t *, output_t *, penv_t *, 
			xpub_status_cb_t , CLOSURE) const;
  bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;
  void output (output_t *o, penv_t *e) const;
protected:
private:
  str _iter;
  str _arr;
  ptr<nested_env_t> _env;
};

//-----------------------------------------------------------------------

class pfile_include2_t : public pfile_include_t {
public:
  pfile_include2_t (int l) : pfile_include_t (l) {}
  pfile_include2_t (const xpub_include_t &x);
  virtual ~pfile_include2_t () {}
  virtual void output (output_t *o, penv_t *e) const;
  virtual bool add (ptr<arglist_t> l);
  virtual bool validate ();
  virtual void explore (pub_exploremode_t mode) const {}
  pfile_el_type_t get_type () const { return PFILE_INCLUDE2; }
  virtual str get_obj_name () const { return "pfile_include2_t"; }
  virtual bool to_xdr (xpub_obj_t *x) const;
  void dump2 (dumper_t *d) const;

  // For pub2
  virtual void publish (pub2_iface_t *, output_t *, penv_t *, 
			xpub_status_cb_t , CLOSURE) const;
  virtual bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;

protected:
  virtual bool to_xdr_base2 (xpub_obj_t *x, xpub_obj_typ_t typ) const;
  // evaluate filename
  str eval_fn (penv_t *env) const;

  ptr<pstr_t> fn_v2;
};

class pfile_load_t : public pfile_include2_t {
public:
  pfile_load_t (int l) : pfile_include2_t (l) {}
  pfile_load_t (const xpub_include_t &x);
  bool muzzle_output () const { return true; }
  virtual bool to_xdr (xpub_obj_t *x) const;
};

class pfile_g_init_pdl_t : public pfile_func_t {
public:
  pfile_g_init_pdl_t (int l, const pfile_t *const f) 
    : pfile_func_t (l), file (f), err (false) {}
  void output (output_t *o, penv_t *e) const;
  bool add (ptr<arglist_t> l);
  bool validate () { return (!err); }
  void dump2 (dumper_t *d) const {}
  str get_obj_name () const { return "pfile_g_init_pdl_t"; }
protected:
  const pfile_t *const file;
  bool err;
};

class pfile_g_ctinclude_t : public pfile_include_t {
public:
  pfile_g_ctinclude_t (int l) : pfile_include_t (l) {}
  virtual void output (output_t *o, penv_t *e) const;
  virtual bool add (ptr<arglist_t> l);
  bool validate () { return (!err); }
  virtual bool ct_read () const { return true; }

  virtual void dump2 (dumper_t *d) const;
  virtual str get_obj_name () const { return "pfile_g_ctinclude_t"; }
protected:
  str osink;
};

class pfile_g_include_t : public pfile_g_ctinclude_t {
public:
  pfile_g_include_t (int l) : pfile_g_ctinclude_t (l) {}
  void output (output_t *o, penv_t *e) const;
  bool add (ptr<arglist_t> l);
  void explore (pub_exploremode_t mode) const {}
  bool ct_read () const { return false; }
  void dump2 (dumper_t *d) const;
  virtual str get_obj_name () const { return "pfile_g_include_t"; }
private:
  str pubobj;
};

class pfile_gframe_t : public pfile_frame_t, public pfile_el_t  {
public:
  pfile_gframe_t (gvars_t *g) : vars (g) {}
  void output (output_t *o, penv_t *e) const { push_frame (e, NULL, vars); }
  ~pfile_gframe_t () { if (vars) delete vars; }
  pfile_el_type_t get_type () const { return PFILE_GFRAME; }
  void dump2 (dumper_t *d) const { if (vars) vars->dump (d); }
private:
  gvars_t *vars;
};

class pfile_html_sec_t : public pfile_sec_t {
public:
  pfile_html_sec_t (int l, bool n = false) : pfile_sec_t (l), nlgobble (n) {}
  pfile_html_sec_t (const xpub_section_t &x);
  pfile_html_sec_t *add (char c); 
  pfile_html_sec_t *add (const str &s);
  str get_obj_name () const { return "pfile_html_sec_t"; }
  bool is_empty () const { return (!els || els->is_empty ()); }
  bool to_xdr (xpub_section_t *x) const;
  inline void hadd_space ();
protected:
  bool nlgobble;
};

class pfile_html_tag_t : public pfile_html_sec_t {
public:
  pfile_html_tag_t (int l, const str &t) 
    : pfile_html_sec_t (l), open (t.len () < 2 || t.cstr () [1] != '/')
  {
    add (t);
    btag_set (true);
  }
  bool open;
  bool is_tag () const { return true; }
};

class pfile_raw_el_t : public pfile_el_t {
public:
  pfile_raw_el_t (const str &h) : _dat (h) {}
  pfile_raw_el_t (const xpub_raw_t &r) : _dat (r.dat.base (), r.dat.size ()) {}
  bool to_xdr (xpub_obj_t *x) const;
  pfile_el_type_t get_type () const { return PFILE_RAW; }
  void output (output_t *o, penv_t *e) const;
  str get_obj_name () const { return "pfile_raw_el_t"; }
private:
  str _dat;
};

class pfile_html_el_t : public pfile_el_t, public concatable_str_t {
public:
  pfile_html_el_t (const str &h) : concatable_str_t (h) {}
  pfile_html_el_t (char c) : concatable_str_t (c) {}
  pfile_html_el_t (const xpub_html_el_t &x) 
    : concatable_str_t (xdr_to_zstr (x.data)) {}
  pfile_el_type_t get_type () const { return PFILE_HTML_EL; }
  void output (output_t *o, penv_t *e) const;
  bool to_xdr (xpub_obj_t *x) const;

  str get_obj_name () const { return "pfile_html_el_t"; }
  void dump2 (dumper_t *d) const;
};

class pfile_gprint_t : public pfile_html_sec_t {
public:
  pfile_gprint_t (int l, const str &o) 
    : pfile_html_sec_t (l, true), outv (o) {}
  void output (output_t *o, penv_t *e) const;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pfile_gprint_t"; }
private:
  const str outv;
};

class pfile_code_t : public pfile_sec_t {
public:
  void output (output_t *o, penv_t *e) const;
  pfile_code_t (int l) : pfile_sec_t (l, false) {}
  pfile_sec_t *add (char c) { sb.tosuio ()->copy (&c, 1); return this; }
  pfile_sec_t *add (const str &s) { sb << s; return this; }
  pfile_el_type_t get_type () const { return PFILE_CCODE; }

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pfile_code_t"; }
private:
  strbuf sb;
};

/* serves mainly as a traverser class */
class pfile_pstr_t : public pfile_el_t {
public:
  pfile_pstr_t (const xpub_file_pstr_t &x) : 
    pstr (New refcounted<pstr_t> (x.pstr)) {}
  pfile_pstr_t (ptr<pstr_t> s) : pstr (s) {}
  ~pfile_pstr_t () {}
  void output (output_t *o, penv_t *e) const {}
  pfile_el_type_t get_type () const { return PFILE_PSTR; }
  bool sec_traverse_init () { return true; }
  pfile_el_t *sec_traverse ();

  str get_obj_name () const { return "pfile_pstr_t"; }
  void dump2 (dumper_t *d) const { pstr->dump (d); }
  bool to_xdr (xpub_obj_t *x) const;
private:
  ref<pstr_t> pstr;
};

class pfile_t : public virtual dumpable_t, 
		public virtual pub2able_t {
public:
  pfile_t (const phashp_t &h, pfile_type_t t);
  pfile_t (const xpub_file_t &x);
  virtual ~pfile_t () { secs.deleteall (); }
  void lex_activate (pfile_type_t t);
  void lex_reactivate ();
  int inc_lineno (int n = 1) { return (lineno += n); }
  int get_lineno () const { return lineno; }
  void output (output_t *o, penv_t *e) const;
  pfile_type_t type () const { return pft; }

  // section management tools
  void add_section (pfile_sec_t *s = NULL) ;
  void add_section2 (pfile_sec_t *s = NULL); /* nests section in section */
  void push_section (pfile_sec_t *s) ;
  pfile_sec_t *pop_section ();

  void to_xdr (xpub_file_t *x) const;

  str get_obj_name () const { return "pfile_t"; }
  void dump2 (dumper_t *d) const;
  void explore (pub_exploremode_t mode) const ;
  void setfp (FILE *f);
  void close () { fclose (fp); fp = NULL; }

  void add_ifile (const pfnm_t &n) { ifiles.push_back (n); }
  const vec<pfnm_t> & get_ifiles () const { return ifiles; }

  mutable ihash_entry<pfile_t> hlink;

  pfile_sec_lst_t secs;
  pubstat_t err;
  str err_msg;
  const phashp_t hsh; 

  // for pub2
  void publish (pub2_iface_t *iface, output_t *o, penv_t *genv,
		xpub_status_cb_t cb, CLOSURE) const;
  PUBLISH_BLOCKS();
private:
  int lineno;
  yy_buffer_state *yybs;
  FILE *fp;
  pfile_type_t pft;
  vec<pfile_sec_t *> stack;
  vec<pfnm_t> ifiles;

public:
  pfile_sec_t *section;

  // for chunked file xfer in pub2
  void init_xdr_opaque ();
  ssize_t len () const;
  ssize_t get_chunk (size_t offset, char *buf, size_t capacity) const;
  void get_xdr_hash (xpubhash_t *x) const { _xdr_opaque_hash.to_xdr (x); }
protected:
  str _xdr_opaque;
  phash_t _xdr_opaque_hash;
};

typedef ihash<const phashp_t, pfile_t, &pfile_t::hsh, 
	      &pfile_t::hlink> pfile_map_t;

class pfile_gs_t : public pfile_sec_t {
public:
  pfile_gs_t (int l) : pfile_sec_t (l) {}
  virtual void output (output_t *o, penv_t *e) const;
  virtual str get_obj_name () const { return "pfile_gs_t"; }
private:
  pfile_frame_t frm;
};

class pvar_t 
  : public virtual evalable_t, public virtual refcount, 
    public virtual dumpable_t {
public:
  pvar_t (const str &n, int l = 0) : nm (n), lineno (l), val (NULL) {}
  pvar_t (pval_t *v) : nm ("**Anonymous PVAR**"), lineno (0), val (v) {}

  virtual void eval_obj (pbuf_t *s, penv_t *e, u_int depth) const;
  virtual str get_obj_name () const { return "pvar_t"; }

  void dump2 (dumper_t *d) const;
  void output (output_t *o, penv_t *e) const;
  int get_lineno () const { return lineno; }
  str name () const { return nm; }

protected:
  const str nm;
  int lineno;
  pval_t *val;
};

class gcode_t : public pvar_t {
public:
  gcode_t (const str &v, int l = 0, bool es = false) 
    : pvar_t (v, l), escaped (false) {}
  void eval_obj (pbuf_t *s, penv_t *e, u_int depth = P_INFINITY) const;
  str get_obj_name () const { return "gcode_t"; }
  bool escaped;
};

class pfile_var_t : public pfile_el_t
{
public:
  pfile_var_t (ptr<pvar_t> v, int l = 0) : var (v), lineno (l) {}
  pfile_var_t (const xpub_file_var_t &x);
  void output (output_t *o, penv_t *e) const; 
  pfile_el_type_t get_type () const { return PFILE_VAR; }
  str get_obj_name () const { return "pfile_var_t"; }
  void dump2 (dumper_t *d) const { var->dump (d); }
  bool to_xdr (xpub_obj_t *x) const;
private:
  const ptr<pvar_t> var;
  const int lineno;
};

class pswitch_env_nullkey_t;

class pswitch_env_base_t : public virtual dumpable_t ,
			   public virtual refcount {
public:
  pswitch_env_base_t (const pfnm_t &n, ptr<aarr_arg_t> a,
		      ptr<nested_env_t> ne)
    : fn (n), aarr (a), _nested_env (ne) {}
  pswitch_env_base_t (const xpub_switch_env_base_t &x);

  static ptr<pswitch_env_base_t> alloc (const xpub_switch_env_union_t &x);

  virtual ~pswitch_env_base_t () {}
  aarr_t *env () const;
  pfnm_t filename () { return fn; }

  void dump2 (dumper_t *d) const;
  ptr<nested_env_t> nested_env () { return _nested_env; }
  virtual bool match (const scalar_obj_t &s) const = 0;
  virtual bool to_xdr (xpub_switch_env_union_t *x) const = 0;
  virtual str get_key () const = 0;
  virtual bool is_exact () const { return false; }
  bool to_xdr_base (xpub_switch_env_base_t *b) const;
  str get_obj_name () const { return "pswitch_env_t"; }
  virtual ptr<pswitch_env_nullkey_t> to_nullkey () { return NULL; }

  const pfnm_t fn;
private:
  const ptr<aarr_arg_t> aarr;
  ptr<nested_env_t> _nested_env;
};

class pswitch_env_nullkey_t : public pswitch_env_base_t {
public:
  pswitch_env_nullkey_t (const pfnm_t &n, ptr<aarr_arg_t> a,
			 ptr<nested_env_t> ne)
    : pswitch_env_base_t (n, a, ne) {}
  str get_key () const { return "<NULL>"; }
  pswitch_env_nullkey_t (const xpub_switch_env_nullkey_t &k);
  bool match (const scalar_obj_t &s) const { return false; }
  bool to_xdr (xpub_switch_env_union_t *e) const;
  ptr<pswitch_env_nullkey_t> to_nullkey () { return mkref (this); }
};

class pswitch_env_exact_t : public pswitch_env_base_t {
public:
  pswitch_env_exact_t (const str &k, const pfnm_t &n, ptr<aarr_arg_t> a,
		       ptr<nested_env_t> ne)
    : pswitch_env_base_t (n, a, ne), _key (k) {}
  pswitch_env_exact_t (const xpub_switch_env_exact_t &x);

  bool to_xdr (xpub_switch_env_union_t *e) const;
  bool match (const scalar_obj_t &s) const;
  str get_key () const { return _key; }

  virtual bool is_exact () const { return true; }
  const str _key;
};

class pswitch_env_rxx_t : public pswitch_env_base_t {
public:
  pswitch_env_rxx_t (ptr<pub_regex_t> rxx, const pfnm_t &n, 
		     ptr<aarr_arg_t> a, ptr<nested_env_t> ne)
    : pswitch_env_base_t (n, a, ne), _rxx (rxx) {}
  pswitch_env_rxx_t (const xpub_switch_env_rxx_t &x);
  str get_obj_name () const { return "pswitch_env_rxx_t"; }
  bool match (const scalar_obj_t &s) const;

  str get_key () const { return _rxx->to_str (); }
  bool to_xdr (xpub_switch_env_union_t *e) const;
  const ptr<pub_regex_t> _rxx;
};

class pswitch_env_range_t : public pswitch_env_base_t {
public:
  pswitch_env_range_t (ptr<pub_range_t> r, const pfnm_t &n,
		       ptr<aarr_arg_t> a, ptr<nested_env_t> ne)
    : pswitch_env_base_t (n, a, ne), _range (r) {}
  pswitch_env_range_t (const xpub_switch_env_range_t &x);
  str get_obj_name () const { return "pswitch_env_range_t"; }
  bool match (const scalar_obj_t &s) const;
  str get_key () const { return _range->to_str (); }
  bool to_xdr (xpub_switch_env_union_t *e) const;
  const ptr<pub_range_t> _range;
};


template<class T> struct keyfn<T, phashp_t> {
  keyfn () {}
  const phashp_t & operator () (T *e) const
  { return (e->hsh); }
};


class pfile_switch_t : public pfile_func_t {
public:
  pfile_switch_t (int l) : pfile_func_t (l), err (false), def (NULL), 
			   key (NULL), nulldef (false), nullcase (NULL),
			   _cached_eval_res (NULL), 
			   _eval_cache_flag (false) {}
  pfile_switch_t (const xpub_switch_t &x);
  ~pfile_switch_t ();
  void output (output_t *o, penv_t *e) const;

  ptr<pswitch_env_base_t> eval_for_output (output_t *o, penv_t *e) const;

  bool add (ptr<arglist_t> a);
  bool validate ();
  void explore (pub_exploremode_t mode) const;
  bool to_xdr (xpub_obj_t *x) const;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pfile_switch_t"; }
    
  // For pub2
  void publish (pub2_iface_t *, output_t *, penv_t *, xpub_status_cb_t ,
		CLOSURE) const;
  bool publish_nonblock (pub2_iface_t *, output_t *, penv_t *) const;

private:
  bool add_case(ptr<arglist_t> l);
  vec<pfnm_t> files;
  bool err;
  ptr<pswitch_env_nullkey_t> def;
  ptr<pvar_t> key;
  bool nulldef;
  ptr<pswitch_env_nullkey_t> nullcase;
  mutable ptr<pswitch_env_base_t> _cached_eval_res;
  mutable bool _eval_cache_flag;

  vec<ptr<pswitch_env_base_t> > _all_cases;

  // Cases w/ exact matches, lookup by key.
  qhash<str, ptr<pswitch_env_base_t> > _exact_cases;
  
  // All cases keyed by RXXs. Note we're using ptr's here instead of
  // low-level *'s just for convenience.
  vec<ptr<pswitch_env_base_t> > _other_cases;
};

//-----------------------------------------------------------------------

class aarr_arg_t : public aarr_t, public pval_t {
public:
  aarr_arg_t (const xpub_aarr_t &x) : aarr_t (x) {}
  aarr_arg_t () {}
  ptr<aarr_arg_t> to_aarr () { return mkref (this); }
  ptr<const aarr_arg_t> to_aarr () const { return mkref (this); }
  str get_obj_name () const { return "aarr_arg_t"; }
  void eval_obj (pbuf_t *, penv_t *, u_int) const {}
};

//-----------------------------------------------------------------------

class nested_env_t : public arg_t, public pub2able_t
{
public:
  nested_env_t (pfile_sec_t *s) : _sec (s) {}
  ~nested_env_t () { delete _sec; }
  str get_obj_name () const { return "nested_env_t"; }
  void eval_obj (pbuf_t *, penv_t *, u_int) const {}
  void dump2 (dumper_t *d) const {}
  ptr<nested_env_t> to_nested_env () { return mkref (this); }
  const pfile_sec_t *sec () const { return _sec; }
  void output (output_t *o, penv_t *e) const;
  static ptr<nested_env_t> alloc (const xpub_section_t &s);

  // For pub2
  void publish (pub2_iface_t *i, output_t *o, penv_t *g, xpub_status_cb_t cb,
		CLOSURE) const
  { _sec->publish (i, o, g, cb); }

  bool publish_nonblock (pub2_iface_t *i, output_t *o, penv_t *g) const
  { return _sec->publish_nonblock (i, o, g); }
private:
  pfile_sec_t *_sec;
  pfile_frame_t _frm;
};

class pfile_set_func_t : public pfile_func_t {
public:
  pfile_set_func_t (int l) 
    : pfile_func_t (l), err (false), env (NULL) {}
  ~pfile_set_func_t () {}
  pfile_set_func_t (const xpub_set_func_t &x);
  virtual void output (output_t *o, penv_t *e) const;
  bool add (ptr<arglist_t> a);
  bool validate () { return true; }
  virtual void output_runtime (penv_t *e) const;
  void output_config (penv_t *e) const ;

  void dump2 (dumper_t *d) const;
  virtual str get_obj_name () const { return "pfile_set_func_t"; }
  virtual bool to_xdr (xpub_obj_t *x) const;
  ptr<aarr_arg_t> get_aarr () const { return aarr; }
protected:
  bool to_xdr_common (xpub_obj_t *x, xpub_obj_typ_t typ) const;
  bool err;
  ptr<aarr_arg_t> aarr;
  mutable penv_t *env;
};

class pfile_set_local_func_t : public pfile_set_func_t {
public:
  pfile_set_local_func_t (int l) : pfile_set_func_t (l) {}
  pfile_set_local_func_t (const xpub_set_func_t &x)
    : pfile_set_func_t (x) {}
  str get_obj_name () const { return "pfile_set_local_func_t"; }
  bool to_xdr (xpub_obj_t *x) const;
  void output_runtime (penv_t *e) const;
};

class pval_null_t : public pval_t {
public:
  void eval_obj (pbuf_t *p, penv_t *e, u_int d) const {}
  bool is_null () const { return true; }
  str get_obj_name () const { return "pval_null_t"; }
  bool to_xdr (xpub_val_t *x) const;
};

class pbuf_el_t : public virtual concatable_t {
public:
  pbuf_el_t () {}
  virtual ~pbuf_el_t () {}
  virtual str to_str_2 (pub_evalmode_t m) const { return NULL; }
  virtual void output (output_t *o, penv_t *e) const {};
  clist_entry<pbuf_el_t> lnk;
};

class pbuf_var_t : public pbuf_el_t {
public:
  pbuf_var_t (const str &v) : pbuf_el_t (), val (v) {}
  void output (output_t *o, penv_t *e) const { o->output (e, val, false); }
  str to_str_2 (pub_evalmode_t m) const;
  const str val;
};

class pbuf_str_t : public pbuf_el_t, public concatable_str_t {
public:
  pbuf_str_t (const str &v) : pbuf_el_t (), concatable_str_t (v) {}
  void output (output_t *o, penv_t *e) const 
  { o->output (e, to_str(), true); }
  str to_str_2 (pub_evalmode_t m) const;
};

/**
 * A class that is use as an intermediary representation during output;
 * a buf is roughty a string buffer, but holds onto zbuf compression
 * information.  pval_t's are evaluated into pbuf_t's, stripping out
 * all ${VAR}-type instances and replacing them with their 
 * resolved values.  It's also a pval_t, since we're allowing them
 * to be stored as pval_t's in aarr_t's, after the aarr_t's have been
 * flattened for pub2.
 */
class pbuf_t : public pval_t {
public:
  pbuf_t () : n (0) {}
  ~pbuf_t () { els.deleteall (); }
  void output (output_t *o, penv_t *e) const;
  str to_str (pub_evalmode_t m = EVAL_FULL, bool allownull = false) const;
  void add (pbuf_el_t *v);
  void add (const str &s);
  void add (zbuf *z);

  void eval_obj (pbuf_t *b, penv_t *e, u_int m) const ;
  str get_obj_name () const { return "pbuf_t"; }

private:
  int n;
  publist_t<pbuf_el_t, &pbuf_el_t::lnk> els;
};

/**
 * Allows a pbuf to be treated as a pbuf_el_t, and to be inserted into
 * a pbuf in a nested fashion.
 */
class pbuf_connector_t : public pbuf_el_t {
public:
  pbuf_connector_t (ptr<const pbuf_t> p) : _pbuf (p) {}
  str to_str_2 (pub_evalmode_t m) const { return _pbuf->to_str (m); }
  void output (output_t *o, penv_t *e) const { return _pbuf->output (o,e); }
private:
  ptr<const pbuf_t> _pbuf;
};

class pbuf_zbuf_t : public pbuf_el_t {
public:
  pbuf_zbuf_t (zbuf *z) : pbuf_el_t (), zb (z) {}
  void output (output_t *o, penv_t *e) const
  { o->output (e, zb, true); }
  str to_str_2 (pub_evalmode_t m) const;
  void eval_obj (pbuf_t *p,  penv_t *e, u_int d) const { p->add (zb); }
private:
  zbuf *zb;
};

class pint_t : public pval_t {
public:
  str eval_simple () const { str s = str (strbuf () << val); return s; }
  pint_t (int i) : val (i) {}
  void eval_obj (pbuf_t *p, penv_t *e, u_int d) const 
  { p->add (eval_simple ()); }

  str get_obj_name () const { return "pint_t"; }
  void dump2 (dumper_t *d) const { DUMP(d, "val: " << val); }
  bool to_xdr (xpub_val_t *x) const;
  bool to_int64 (int64_t *i) const { *i = val; return true; }

  ptr<pval_t> flatten (penv_t *) ;
private:
  int val;
};

class pstr_var_t : public pstr_el_t {
public:
  void eval_obj (pbuf_t *s, penv_t *e, u_int d) const 
  { pvar->eval_obj (s, e, d); }
  void output (output_t *o, penv_t *e) const { assert (false); }
  pstr_var_t (ptr<pvar_t> v) : pvar (v) {}
  pstr_var_t (const xpub_var_t &x) : pvar (New refcounted<pvar_t> (x)) {}
  pfile_el_t *to_pfile_el () { return New pfile_var_t (pvar); }
  void dump2 (dumper_t *d) const { if (pvar) pvar->dump (d); }
  str get_obj_name () const { return "pstr_var_t"; }
  bool to_xdr (xpub_pstr_el_t *x) const;
private:
  const ptr<pvar_t> pvar;
};

class pstr_str_t : public pstr_el_t, public concatable_str_t {
public:
  pstr_str_t (const str &s) : concatable_str_t (s) {}
  pstr_str_t (char c) : concatable_str_t (c) {}
  pstr_str_t (const xpub_str_t &x) : concatable_str_t (x) {}
  void eval_obj (pbuf_t *p, penv_t *e, u_int d) const { p->add (to_str ()); }
  void output (output_t *o, penv_t *e) const 
  { o->output (e, to_str (), true); }
  pfile_el_t *to_pfile_el () { return New pfile_html_el_t (to_str ()); }
  void dump2 (dumper_t *d) const { DUMP(d, "str: " << to_str ()); }
  str get_obj_name () const { return "pstr_str_t"; }
  bool to_xdr (xpub_pstr_el_t *x) const;
};

/* in pub.C  -- high level pub datatypes */

class pub_base_t {
public:
  pub_base_t (u_int o = 0) : mcf (NULL), _opts (o) {}
  virtual ~pub_base_t () { if (mcf) delete mcf; }
  
  bool run_configs ();
  bool run_config (pfile_t *f);
  bool run_config (str nm);

  inline void set_opts (u_int o) { _opts = o; }
  inline u_int get_opts () const { return _opts; }

  virtual bpfcp_t v_getfile (const pfnm_t &n) const = 0;
  virtual u_int fixopts (u_int in) const { return in; }

  bool include (zbuf *b, const pfnm_t &fn, u_int opts = 0, 
		aarr_t *env = NULL) const;
  bool include (zbuf *b, bpfcp_t f, u_int o = 0, aarr_t *e = NULL) const;
  void r_config (pubrescb , int mthd, ptr<aclnt> );
  void configed (ptr<xpub_getfile_res_t> x, pubrescb c, clnt_stat e);
  virtual void v_config_cb (const xpub_file_t &x) {}

protected:
  vec<str> cfgfiles;     // all root cfg files
  pfile_t *mcf; // master config file
  mutable penv_t genv;   // global eval env

private:
  u_int _opts;     // options for publishing!
};

// A wrapper opbject around bound pfiles, that keeps everything in scope.
class bound_pfile2_t {
public:
  bound_pfile2_t (pfnm_t nm, const xpub2_fstat_t &stat, 
		  const xpub_file_t &file, u_int o)
    : _name_in (nm),
      _name_out (stat.fn),
      _hsh (phash_t::alloc (stat.hash)),
      _binding (New refcounted<pbinding_t> (_name_out, _hsh)),
      _file (file),
      _bpf (bound_pfile_t::alloc (_binding, &_file, _name_in)),
      _ctime (stat.ctime),
      _opts (o) {}
  
  bound_pfile2_t (ptr<pbinding_t> bnd, pfnm_t jnm, pfile_type_t t, u_int o)
    : _name_in (bnd->filename ()),
      _name_out (bnd->filename ()),
      _hsh (bnd->hash ()),
      _binding (bnd),
      _file (_hsh, t),
      _bpf (bound_pfile_t::alloc (_binding, &_file, jnm, false)),
      _opts (o) {}
      
  bpfcp_t bpf () const { return _bpf; }
  bpfmp_t nonconst_bpf () const { return _bpf; }
  void set_bpf (ptr<bound_pfile_t> b) { _bpf = b; }
  pfile_t *file () { return &_file; }
  time_t ctime () const { return _ctime; }
  phashp_t hash () const { return _hsh; }
  void set_ctime (time_t t) { _ctime = t; }
  const str &filename () const { return _name_out; }
  u_int opts () const { return _opts; }
  
private:
  const str _name_in;
  const str _name_out;
  phashp_t _hsh;
  ptr<pbinding_t> _binding;
  pfile_t _file;
  ptr<bound_pfile_t> _bpf;
  time_t _ctime;
  u_int _opts;
};

/*
 * pubv2: The streamlined interface with the publisher, for accessing
 * the internal config variables in a publishing after publishing
 * a config file
 */
class pub_config_iface_t {
public:
  virtual ~pub_config_iface_t () {}
  virtual penv_t * get_env () const = 0;
  str cfg (const str &n, bool allownull = false) const;
  bool cfg (const str &n, const pval_t **v) const;
  bool cfg (const str &n, str *v, bool allownull = false) const;
  template<typename T> bool cfg (const str &n, T *v) const;
  pval_w_t operator[] (const str &s) const { return (*get_env ())[s]; }
};

/*
 * pubv1 cruft: a lot of this stuff can be removed once pub v1 is
 * phased out.
 */
class pub_t : public pub_base_t, public pub_config_iface_t {
public:
  pub_t (u_int o = 0) : pub_base_t (o), set (New set_t ()), rebind (false) {}
  virtual ~pub_t () { delete set; }

  // V1 Only
  bpfcp_t getfile (const pfnm_t &nm) const ;   
  pfile_t *getfile (phashp_t hsh) const;
  bool add_rootfile (const str &fn, bool conf = false);
  void queue_hfile (const pfnm_t &n) { parsequeue.push_back (n); }
  virtual void explore (const str &fn) const { assert (false); }
  bpfcp_t v_getfile (const pfnm_t &n) const
  { return set->getfile (apply_homedir (n)); }


  // V1 and V2
  virtual str jail2real (const str &n) const { return n; }
  void set_homedir (const str &d) { homedir = dir_standardize (d); }
  pfnm_t apply_homedir (const pfnm_t &n) const;

  // on for all except version 2 pub_parser_t
  virtual bool do_explore () const { return true; }
  virtual xpub_version_t get_version () const { return XPUB_V1; }

  penv_t *get_env () const { return &genv; }
  const penv_t *get_const_env () const { return &genv; }

  bool do_wss () const { return (get_opts () & P_WSS); }
  bool be_verbose () const { return (get_opts () & P_VERBOSE); }

  // V1 only
  struct set_t {
    set_t () {}
    ~set_t () { files.deleteall (); }
    void insert (const pbinding_t *bnd, const pfile_t *f = NULL);
    void insert (const pfile_t *f);
    void remove (pfile_t *f);
    void remove (const pbinding_t *bnd, pfile_t *f);
    void insert (bpfcp_t bpf);
    bpfcp_t getfile (const pfnm_t &nm, const set_t *backup = NULL) const;
    pbinding_t *to_binding (const pfnm_t &fn, const pfnm_t &rfn, 
			    bool rebind, bool toplev = false);
    pbinding_t *alloc (const pfnm_t &fn, phashp_t h, bool toplev = false);

    bindtab_t  bindings;                // expands filenames to hashes
    pfile_map_t files;                  // expands hashes to files
  };
  virtual const set_t *get_backup_set () const { return NULL; }

  vec<str> rootfiles;     // all pub rootfiles (V1)
  bhash<str> rfbh;        // rootfile bhash (V1)
  bhash<str> crfbh;       // config rootfile bhash (V1)
  set_t *set;             // working set of files (V1)
  vec<pfnm_t> parsequeue; // for synchronous file exploration (V1)
  bool rebind;            // manage binding tables locally (V1)

  str homedir;
};

// V1 cruft
class pub_client_t : public pub_t 
{
public:
  pub_client_t () : pub_t (), nset (NULL) {}
  void explore (const pfnm_t &fn) const;
  static pub_client_t *alloc ();
  virtual const set_t *get_backup_set () const { return nset; }
  set_t *nset;
};

// V1 cruft
class pub_rclient_t : public pub_client_t
{
public:
  pub_rclient_t (ptr<aclnt> c, int lm, int gfm, int pcm, u_int am = ~0, 
		 u_int om = 0) 
    : pub_client_t (), clnt (c), lookup_mthd (lm), getfile_mthd (gfm),
      pubconf_mthd (pcm), running (false), nreq (0), init (false), 
      andmask (am), ormask (om) 
  { 
    set_homedir ("/"); 
  }
  void publish (pubrescb c) { publish (rootfiles, c); }
  void publish (const vec<str> &files, pubrescb c);
  void publish (const xpub_fnset_t &files, pubrescb c);
  void config (pubrescb c) { r_config (c, pubconf_mthd, clnt); }
  static pub_rclient_t *alloc (ptr<aclnt> c, int lm, int gm, int cm,
			       u_int am = ~0, u_int = 0);

  struct waiter_t {
    waiter_t (const vec<str> &f, pubrescb c) 
      : files (f), cb (c) {}
    vec<str> files;
    pubrescb cb;
  };
  u_int fixopts (u_int in) const { return ((in & andmask) | ormask); }

private:

  void getfile (const pfnm_t &fn);
  void lookup (const pfnm_t &fn, bindcb c);
  void lookedup (pfnm_t fn, ptr<xpub_lookup_res_t> xr, clnt_stat err);
  void getfile_lookedup (pbinding_t *bnd);
  void getfile (pbinding_t *bnd);
  void gotfile (ptr<xpub_getfile_res_t> xr, pbinding_t *bnd, clnt_stat err);
  void finish_req ();
  void finish_publish ();
  bool new_set ();
  void explore (const pfnm_t &fn) const;

  ptr<aclnt> clnt;
  const int lookup_mthd;
  const int getfile_mthd;
  const int pubconf_mthd;
  ptr<pub_res_t> res;
  vec<waiter_t *> waiters;
  bhash<phashp_t> getfile_hold;
  holdtab_t<pfnm_t, bindcb, pbinding_t *> holdtab;
  pubrescb::ptr cb;
  bool running;
  int nreq;
  bool init;
  u_int andmask;
  u_int ormask;
};

// V1 cruft
class pub_proxy_t : public pub_base_t 
{
public:
  pub_proxy_t () 
    : pub_base_t (), bindings (wrap (this, &pub_proxy_t::remove)) {}
  static pub_proxy_t *alloc ();
  bool lookup (const xpub_fn_t &f, xpub_lookup_res_t *res);
  bool getfile (const phashp_t &h, xpub_getfile_res_t *res);
  void cache (const xpub_set_t &st);
  void cache (ptr<xpub_file_t> res);
  void cache (const xpub_file_t &f);
  void cache (const xpub_pbinding_t &bnd);
  void cache (const pfnm_t &fn, phashp_t hsh);
  void cache_pubconf (const xpub_file_t &f);
  ptr<xpub_file_t> get_pubconf () const { return pconf; }
  void clear ();
  void remove (phashp_t h);
  bpfcp_t v_getfile (const pfnm_t &nm) const;
  void gc_orphans ();
  void gc_files_iterator (bhash<phashp_t> *in_use, const phashp_t &key,
			  ptr<xpub_file_t> *dummy);

protected:
  void v_config_cb (const xpub_file_t &x) { cache_pubconf (x); }

private:
  bindtab_t bindings;
  ptr<xpub_file_t> pconf;
  qhash<phashp_t, ptr<xpub_file_t> > files;
  qhash<phashp_t, ptr<xpub_file_t> > recycle;
  mutable qhash<pfnm_t, bpfcp_t> my_cache;
};


// V1 and V2; still useful; interface with lex/yacc and parse incoming
// pub-formatted files
class pub_parser_t : public pub_t 
{
public:
  pub_parser_t (bool exp = false) 
    : pub_t (exp ? P_EXPORTER : 0), gvars (NULL), tag (NULL), 
      lasttag (NULL), 
      space_flag (false), xset_collect (false), 
      jaildir (""), jailed (false), jm (JAIL_NONE) {}
  
  static pub_parser_t *alloc (bool exp = false);
  bool init (const str &fn);
  bpfcp_t parse_config (const str &fn, bool init = true);
  bpfcp_t parse (const pbinding_t *bnd, pfile_type_t t);
  void pwarn (const strbuf &b) ;

  void push_file (bpfmp_t f) { if (bpf) stack.push_back (bpf); bpf = f; }
  void pop_file ();
  void dump_stack ();

  bool do_explore () const { return (!(get_opts () & P_EXPLORE_OFF)); }
  bool is_exporter () const { return (get_opts () & P_EXPORTER); }

  void init_set () { xset.clear (); xset_collect = true; }
  void export_set (xpub_set_t *xs);
  void setprivs (str jd, str un, str gn);
  void setjail (str jd, bool permissive = false);
  str jail2real (const str &n) const;

  void lock_in_jail () { jailed = true; }
  void free_from_jail () { if (!(get_opts() & P_DAEMON)) jailed = false; }
  bool behind_bars () const { return (get_opts() & P_DAEMON) || jailed; }

  pbinding_t *to_binding (const pfnm_t &fn, set_t *s = NULL, 
			  bool toplev = false);
  void push_parr (ptr<parr_t> a);
  ptr<parr_t> pop_parr ();

  void set_jailmode (jail_mode_t j) { jm = j; }
  jail_mode_t get_jailmode () const { return jm ; }

  xpub_version_t get_include_version () const 
  { return ((get_opts () & P_INCLUDE_V2) ? XPUB_V2 : XPUB_V1); }

  //
  // manipulate the stack of functions
  //
  pfile_func_t *top_func () { return _func_stack.back (); }
  void push_func (pfile_func_t *f) { _func_stack.push_back (f); }
  pfile_func_t *pop_func () { return _func_stack.pop_back (); }

  str complete_fn (const str &in) const;

  /* global parse variables */
  ptr<pstr_t> pstr;
  ptr<aarr_arg_t> aarr;
  ptr<arglist_t> arglist;
  ptr<parr_t> parr;
  concatable_str_t *str1;
  bpfmp_t bpf;
  gvars_t *gvars;
  vec<pfile_func_t *> _func_stack;
  pfile_html_tag_t *tag, *lasttag;

  bool space_flag;
  int last_tok;

  // for publishing HTML files only, in the world of Pub2
  ptr<bound_pfile2_t> pub2_parse (ptr<pbinding_t> bnd, int opts,
				  pubstat_t *err, str *errmsg);

  ptr<xpub_getfile_res_t> defconf;
private:
  bpfcp_t parse1 (const pbinding_t *bnd, pfile_sec_t *ss, pfile_type_t);
  bpfcp_t parse2 (const pbinding_t *bnd, pfile_sec_t *ss, pfile_type_t);
  vec<bpfmp_t> stack;
  vec<bpfcp_t> xset;
  bool xset_collect;
  str jaildir;
  bool jailed;
  jail_mode_t jm;
  vec<ptr<parr_t> > parr_stack;
};

class cfgw_t {
public:
  cfgw_t (pub_t *p) : pubp (p) {}
  pval_w_t operator[] (const str &s) const { return (*pubp)[s]; }
private:
  pub_t *pubp;
};

extern pub_parser_t *parser;
extern pub_t *pub;
extern pub_client_t *pcli;
extern pub_base_t *pubincluder;

#define TEE(b,s)  b << s; warn << s;
  
void 
pfile_sec_t::apply_space ()
{
  if (parser->space_flag) {
    parser->space_flag = false;
    add (' ');
  }
}

void
pfile_sec_t::hadd_space ()
{
  if (!yywss) add (' ');
  else parser->space_flag = true;
}

void
pfile_html_sec_t::hadd_space ()
{
  if (!yywss) add (' ');
  else if (nlgobble) nlgobble = false;
  else parser->space_flag = true;
}

void
pfile_sec_t::hadd (pfile_sec_t *s)
{
  if (s->is_tag ()) {
    //
    // MK - 11.10.2003 we're going to be conservative here...
    //
    // if (etag_flag && parser->space_flag &&
    //(PLASTHTAG && (PLASTHTAG->open || !PHTAG->open)))
    // parser->space_flag = false;
    etag_flag = true;
  }
  apply_space ();
  add (s);
}

template<typename T> bool 
pub_config_iface_t::cfg (const str &n, T *v) const
{
  str s;
  return (cfg (n, &s) && convertint (s, v));
}

template<class T> aarr_t &
aarr_t::add (const str &n, T i)
{
  add (New nvpair_t (n, pub_scalar_t::alloc (i)));
  return (*this);
}

template<class T> aarr_t &
aarr_t::replace_so (const str &n, T i)
{
  ptr<pval_t> v = pub_scalar_t::alloc (i);
  return replace (n, v);
}

const char * getenvval (const char *s);

#endif /* _LIBPUB_PUB_H */
