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

#include "amisc.h"
#include "vec.h"
#include "qhash.h"
#include "clist.h"
#include "pubutil.h"
#include "arpc.h"
#include "puberr.h"
#include "holdtab.h"
#include "zstr.h"

#define sNULL static_cast<str> (NULL)

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
	       PFILE_FUNC = 10, PFILE_INCLIST = 11 } pfile_el_type_t;

typedef enum { PFILE_TYPE_NONE = 0,
	       PFILE_TYPE_GUY = 1,
	       PFILE_TYPE_H = 2,
	       PFILE_TYPE_WH = 3,
	       PFILE_TYPE_CODE = 4,
	       PFILE_TYPE_EC = 5,
	       PFILE_TYPE_WEC = 6,
               PFILE_TYPE_CONF = 7 } pfile_type_t;

typedef enum { PUBSTAT_OK = 0, PUBSTAT_FNF = 1, 
	       PUBSTAT_PARSE_ERROR = 3 } pubstat_t;

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

/* XXX - defaults should be put someplace better */
#define P_INFINITY   65334

/* shortcut macros for common operations on the global pub object */
#define PWARN(x)   parser->pwarn (strbuf () << x)
#define PFILE      parser->bpf->file
#define PARSEFAIL  PFILE->err = PUBSTAT_PARSE_ERROR;
#define PLINC      PFILE->inc_lineno ()
#define PLINENO    (PFILE->get_lineno ())
#define PSECTION   PFILE->section
#define PFUNC      parser->func
#define ARGLIST    parser->arglist
#define PLASTHTAG  parser->lasttag
#define PHTAG      parser->tag
#define PAARR      parser->aarr
#define PGVARS     parser->gvars
#define PSTR1      parser->str1
#define PPSTR      parser->pstr
#define PARR       parser->parr


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
  bool concat (const str &s) { sb << s; return true; }
  bool concat (strbuf *s) { sb << *s; return true; }
  bool concat (char c) { sb.tosuio ()->copy (&c, 1);  return true; }
  bool concat (concatable_t *l);
  str to_str () const { return sb; }
  const zstr &to_zstr () const { if (z) return z; else return (z = sb); }
private:
  strbuf sb;
  mutable zstr z;
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

class nvpair_t : public virtual dumpable_t {
public:
  nvpair_t (const str &n, ptr<pval_t> v) : nm (n), val (v) {}
  nvpair_t (const xpub_nvpair_t &x);
  virtual ~nvpair_t () {}
  const str &name () const { return nm; }
  pval_t *value () const { return val; }
  void output (output_t *o, penv_t *e, int i = 0) const;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "nvpair_t"; }
  bool to_xdr (xpub_nvpair_t *x) const;

  const str nm;
  ihash_entry<nvpair_t> hlink;
private:
  const ref<pval_t> val;
};

typedef ihash<const str, nvpair_t, &nvpair_t::nm, &nvpair_t::hlink> nvtab_t;

class pval_w_t {
public:
  pval_w_t () : val (NULL), env (NULL), ival_flag (false) {}
  pval_w_t (pval_t *v, penv_t *e) 
    : val (v), int_err (INT_MIN), env (e), ival_flag (false) {}
  pval_w_t (const str &n, penv_t *e)
    : val (NULL), name (n), int_err (INT_MIN), env (e), ival_flag (false) {}
  pval_w_t (int i) : val (NULL), env (NULL), ival_flag (true), ival (i) {}

  inline operator int() const { return to_int (); }
  inline operator str() const { return to_str () ; }
  pval_w_t operator[] (u_int i) const { return elem_at (i); }

  str to_str () const;
  int to_int () const;
  pval_w_t elem_at (u_int i) const;
  u_int size () const;
private:
  const pval_t *get_pval () const;
  pval_t *val;
  const str name;
  const int int_err;
  penv_t *env;
  
  const bool ival_flag;
  const int ival;
};

class aarr_t : public virtual dumpable_t {
public:
  aarr_t () {}
  aarr_t (const xpub_aarr_t &x);
  virtual ~aarr_t () { aar.deleteall (); }
  void add (nvpair_t *p);
  aarr_t &add (const str &n, const str &v);
  aarr_t &add (const str &n, zbuf *b);
  aarr_t &add (const str &n, ptr<zbuf> zb);

  template<class T>
  aarr_t &add (const str &n, T i) {
    strbuf b;
    b << i;
    add (New nvpair_t (n, New refcounted<pstr_t> (b)));
    return (*this);
  }
        
  aarr_t &overwrite_with (const aarr_t &r);
  pval_t *lookup (const str &n);
  void output (output_t *o, penv_t *e) const;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "aarr_t"; }
  bool to_xdr (xpub_aarr_t *x) const;

protected:
  nvtab_t aar;
private:
  friend class penv_t;
  clist_entry<aarr_t> slnk;
};

struct penv_state_t {
  penv_state_t (u_int o, u_int e, bool f) 
    : opts (o), estack_size (e), errflag (f) {}
  u_int opts;
  u_int estack_size;
  bool errflag;
};

class penv_t {
public:

  penv_t (aarr_t *a = NULL, u_int o = 0, aarr_t *g = NULL)
    : aarr_n (1), file (NULL), needloc (false), opts (o), evm (EVAL_FULL),
      olineno (-1), cerrflag (false), tlf (true)
  { 
    if (g) push (g);
    if (a) push (a); 
  }

  ~penv_t () {}

  penv_state_t *start_output (aarr_t *a, u_int o);
  bool finish_output (penv_state_t *s);

  void resize (u_int s);
  void gresize (u_int gvs);
  void resize (u_int s, u_int gvs) { resize (s); gresize (gvs); }
  u_int size () const { return estack.size (); }
  u_int gvsize () const { return gvars.size (); }
  void push (aarr_t *a) { estack.insert_tail (a); }
  void remove (aarr_t *a) { estack.remove (a); }
  void push (const gvars_t *g) { gvars.push_back (g); }
  pval_t *lookup (const str &n, bool recurse = true);
  pval_w_t operator[] (const str &n) { return pval_w_t (n, this); }
  pub_evalmode_t init_eval (pub_evalmode_t m = EVAL_FULL);
  void eval_pop (const str &n);
  void set_evalmode (pub_evalmode_t m) { evm = m; }
  inline str loc (int l = -1) const;
  inline str filename () const;
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
  void clear () { estack.clear (); gvars.clear (); }

  int aarr_n;
  bpfcp_t file;
  bool needloc;
  str cerr;
  u_int opts;
private:
  pub_evalmode_t evm;
  qhash<str, vec<aarr_t *> > evaltab;
  clist_t<aarr_t, &aarr_t::slnk> estack;  // eval stack
  vec<const gvars_t *> gvars;
  vec<bpfcp_t> fstack;
  bhash<phashp_t> istack;
  int olineno;
  bool cerrflag; // compile error flag
  bool tlf; // top level flag
};

class pfile_set_func_t;
class output_t {
public:
  output_t (pfile_type_t m) : mode (m) {}
  virtual ~output_t () {}

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
  virtual bool stack_restore () const { return false; }

  pfile_type_t mode;
};

class output_std_t : public output_t {
public:
  output_std_t (zbuf *o, pfile_type_t m = PFILE_TYPE_H) 
    : output_t (m), out (o), osink_open (false) {}
  output_std_t (zbuf *o, const pfile_t *f = NULL);
  virtual ~output_std_t () {}

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

struct bound_pfile_t : public virtual refcount, public virtual dumpable_t  {
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
  void close ();

  str loc (int i = -1) const;
  phashp_t hash () const { return bnd->hash (); }
  str filename () const { return bnd->filename (); }
  operator bool() const { return (bnd && file); }
  void output (output_t *o, penv_t *e, bpfcp_t kludge) const;
  void explore (pub_exploremode_t m) const; 
  const pbinding_t *const bnd;
  pfile_t *const file;
  const pfnm_t rfn; // used only when publishing 
  bool delfile;
};

class pbuf_t;
class evalable_t {
public:
  virtual str eval (penv_t *e, pub_evalmode_t m = EVAL_FULL) const;
  str eval () const { return eval (NULL, EVAL_SIMPLE); }
  ptr<pbuf_t> eval_to_pbuf (penv_t *e, pub_evalmode_t m) const;
  virtual void eval_obj (pbuf_t *s, penv_t *e, u_int d) const = 0;
  virtual str eval_simple () const { return NULL; }
};

template<class T, clist_entry<T> T::*field>
struct publist_t 
  : public clist_t<T, field>
{
  void output (output_t *o, penv_t *en) const
  {
    for (T *e = first; e; e = next (e)) e->output (o, en); 
  }
  void dump (dumper_t *d) const
  {
    for (T *e = first; e; e = next (e)) e->dump (d);
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
  str get_obj_name () const { return "pfile_frame_t"; }
private:
  mutable int sss;    /* start stack size */
  mutable int sgvss;  /* start gvar stack size */
};

class parr_t;
class parr_mixed_t;
class parr_ival_t;

class arg_t : public virtual refcount, public virtual dumpable_t,
	      public virtual evalable_t
{
public:
  virtual ~arg_t () {}
  virtual ptr<aarr_arg_t> to_aarr () { return NULL; }
  virtual bool is_null () const { return false; }
  virtual ptr<pval_t> to_pval () const { return NULL; }
  virtual bool to_int64 (int64_t *i) const { return false; }
  virtual const parr_mixed_t *to_mixed_arr () const { return NULL; }
  virtual const parr_ival_t *to_int_arr () const { return NULL; }
  virtual const parr_t *to_arr () const { return NULL; }
};

class pval_t : public arg_t {
public:
  virtual ~pval_t () {}
  static ptr<pval_t> alloc (const xpub_val_t &x);
  ptr<pval_t> to_pval () { return mkref (this); }
  virtual bool to_xdr (xpub_val_t *x) const { return false; }
};

class pfile_el_t : public virtual concatable_t, public virtual dumpable_t {
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
    public virtual evalable_t {
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
class pstr_t : public pval_t {
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
protected:
  int n;
  publist_t<pstr_el_t, &pstr_el_t::lnk> els;
};

class pval_zbuf_t : public pval_t {
public:
  // note no to_xdr function
  // pval_zbuf_t (ptr<zbuf> *z) : pval_t (), zb (z), zb_rcp (z) {}
  pval_zbuf_t (zbuf *z) : pval_t (), zb (z) {}
  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "p_zbuf_val_t"; }
  void eval_obj (pbuf_t *ps, penv_t *e, u_int d) const;
  str eval_simple () const { return zb->output (); }
private:
  zbuf *zb;
  //ptr<zbuf> zb_zcp;
};

class pfile_sec_t : public pfile_el_t {
public:
  pfile_sec_t (int l, bool ls = true) 
    : els (ls ? New pfile_el_lst_t () : NULL), lineno (l),
      btag_flag (false), etag_flag (false) {}
  ~pfile_sec_t () { if (els) delete els; }
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

  int get_lineno () const { return lineno; }
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
  pfile_include_t (int l) : pfile_func_t (l), err (false) {}
  pfile_include_t (const xpub_include_t &x);
  virtual ~pfile_include_t () {}
  virtual void output (output_t *o, penv_t *e) const;
  virtual bool add (ptr<arglist_t> l);
  virtual bool validate ();
  void explore (pub_exploremode_t mode) const;
  pfile_el_type_t get_type () const { return PFILE_INCLUDE; }

  virtual void dump2 (dumper_t *d) const;
  virtual str get_obj_name () const { return "pfile_include_t"; }
  bool to_xdr (xpub_obj_t *x) const;
protected:
  bool err;
  pfnm_t fn;
  ptr<aarr_arg_t> env;
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

class pfile_t : public virtual dumpable_t {
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
  const phashp_t hsh;            // all other files don't
private:
  int lineno;
  yy_buffer_state *yybs;
  FILE *fp;
  pfile_type_t pft;
  vec<pfile_sec_t *> stack;
  vec<pfnm_t> ifiles;

public:
  pfile_sec_t *section;
};

typedef ihash<const phashp_t, pfile_t, &pfile_t::hsh, 
	      &pfile_t::hlink> pfile_map_t;

class pfile_ec_gs_t : public pfile_sec_t {
public:
  pfile_ec_gs_t (int l) : pfile_sec_t (l) {}
  virtual void output (output_t *o, penv_t *e) const;
  virtual str get_obj_name () const { return "pfile_ec_gs_t"; }
};

class pfile_gs_t : public pfile_ec_gs_t {
public:
  pfile_gs_t (int lineno) : pfile_ec_gs_t (lineno) {}
  void output (output_t *o, penv_t *e) const;
  str get_obj_name () const { return "pfile_gs_t"; }
private:
  pfile_frame_t frm;
};

class pfile_ec_header_t : public pfile_sec_t {
public:
  pfile_ec_header_t (int l, const str &c, const str &b)
    : pfile_sec_t (l), classname (c), bufmem (b) {}
  void output (output_t *o, penv_t *e) const;
  str get_obj_name () const { return "pfile_ec_header_t"; }
  void dump2 (dumper_t *d) const;
private:
  const str classname;
  const str bufmem;
};

class pfile_ec_main_t : public pfile_sec_t {
public:
  pfile_ec_main_t (int l) : pfile_sec_t (l) {}
  void output (output_t *o, penv_t *e) const;
  str get_obj_name () const { return "pfile_ec_main_t"; }
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

class pswitch_env_t : public virtual dumpable_t {
public:
  pswitch_env_t (const str &k, const pfnm_t &n, ptr<aarr_arg_t> a)
    : key (k), fn (n), aarr (a) {}
  pswitch_env_t (const xpub_switch_env_t &x);
  virtual ~pswitch_env_t () {}
  aarr_t *env () const;
  str filename () { return fn; }

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pswitch_env_t"; }
  bool to_xdr (xpub_switch_env_t *e) const;

  const str key;
  ihash_entry<pswitch_env_t> hlink;
  const pfnm_t fn;
private:
  const ptr<aarr_arg_t> aarr;
};

template<class T> struct keyfn<T, phashp_t> {
  keyfn () {}
  const phashp_t & operator () (T *e) const
  { return (e->hsh); }
};


typedef ihash<const str, pswitch_env_t, &pswitch_env_t::key, 
	      &pswitch_env_t::hlink> cases_t;
class pfile_switch_t : public pfile_func_t {
public:
  pfile_switch_t (int l) : pfile_func_t (l), err (false), def (NULL), 
			   key (NULL), nulldef (false) {}
  pfile_switch_t (const xpub_switch_t &x);
  ~pfile_switch_t () { if (def) delete def; cases.deleteall (); }
  void output (output_t *o, penv_t *e) const;
  bool add (ptr<arglist_t> a);
  bool validate ();
  void explore (pub_exploremode_t mode) const;
  bool to_xdr (xpub_obj_t *x) const;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pfile_switch_t"; }
    
private:
  bool add_case(ptr<arglist_t> l);
  vec<pfnm_t> files;
  bool err;
  cases_t cases;
  pswitch_env_t *def;
  ptr<pvar_t> key;
  bool nulldef;
};

class aarr_arg_t : public aarr_t, public arg_t {
public:
  aarr_arg_t (const xpub_aarr_t &x) : aarr_t (x) {}
  aarr_arg_t () {}
  ptr<aarr_arg_t> to_aarr () { return mkref (this); }
  str get_obj_name () const { return "aarr_arg_t"; }
  void eval_obj (pbuf_t *, penv_t *, u_int) const {}
};

class pfile_set_func_t : public pfile_func_t {
public:
  pfile_set_func_t (int l) 
    : pfile_func_t (l), err (false), env (NULL) {}
  ~pfile_set_func_t () { remove (); }
  pfile_set_func_t (const xpub_set_func_t &x);
  void remove () const;
  void output (output_t *o, penv_t *e) const;
  bool add (ptr<arglist_t> a);
  bool validate () { return true; }
  void output_runtime (penv_t *e) const { push_frame (e, aarr); }
  void output_config (penv_t *e) const ;

  void dump2 (dumper_t *d) const;
  str get_obj_name () const { return "pfile_set_func_t"; }
  bool to_xdr (xpub_obj_t *x) const;
private:
  bool err;
  ptr<aarr_arg_t> aarr;
  mutable penv_t *env;
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

class pbuf_t {
public:
  pbuf_t () : n (0) {}
  ~pbuf_t () { els.deleteall (); }
  void output (output_t *o, penv_t *e) const;
  str to_str (pub_evalmode_t m = EVAL_FULL) const;
  void add (pbuf_el_t *v);
  void add (const str &s);
  void add (zbuf *z);
private:
  int n;
  publist_t<pbuf_el_t, &pbuf_el_t::lnk> els;
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
  pub_base_t () : wss (false), opts (0), mcf (NULL) {}
  virtual ~pub_base_t () { if (mcf) delete mcf; }
  
  bool run_configs ();
  bool run_config (pfile_t *f);
  bool run_config (str nm);

  virtual bpfcp_t v_getfile (const pfnm_t &n) const = 0;
  virtual u_int fixopts (u_int in) const { return in; }

  bool include (zbuf *b, const pfnm_t &fn, u_int opts = 0, 
		aarr_t *env = NULL) const;
  bool include (zbuf *b, bpfcp_t f, u_int o = 0, aarr_t *e = NULL) const;
  void r_config (pubrescb c, int mthd, ptr<aclnt> c);
  void configed (ptr<xpub_getfile_res_t> x, pubrescb c, clnt_stat e);
  virtual void v_config_cb (const xpub_file_t &x) {}

  bool wss;
protected:
  u_int opts;
  vec<str> cfgfiles;     // all root cfg files
  pfile_t *mcf; // master config file
  mutable penv_t genv;   // global eval env

private:
};


class pub_t : public pub_base_t {
public:
  pub_t () : pub_base_t (), set (New set_t ()),  rebind (false) {}
  virtual ~pub_t () { delete set; }

  bpfcp_t getfile (const pfnm_t &nm) const ;
  pfile_t *getfile (phashp_t hsh) const;
  bool add_rootfile (const str &fn, bool conf = false);
  void queue_hfile (const pfnm_t &n) { parsequeue.push_back (n); }
  virtual void explore (const str &fn) const { assert (false); }
  virtual str jail2real (const str &n) const { return n; }
  void set_opts (u_int o) { opts = o; }
  u_int get_opts () const { return opts; }
  void set_homedir (const str &d) { homedir = dir_standardize (d); }
  pfnm_t apply_homedir (const pfnm_t &n) const;

  str cfg (const str &n) const;
  bool cfg (const str &n, pval_t **v) const;
  bool cfg (const str &n, str *v) const;
  template<typename T> bool cfg (const str &n, T *v) const;
  pval_w_t operator[] (const str &s) const { return genv[s]; }

  bpfcp_t v_getfile (const pfnm_t &n) const
  { return set->getfile (apply_homedir (n)); }

  struct set_t {
    set_t () {}
    ~set_t () { files.deleteall (); }
    void insert (const pbinding_t *bnd, const pfile_t *f = NULL);
    void insert (const pfile_t *f);
    void remove (pfile_t *f);
    void remove (const pbinding_t *bnd, pfile_t *f);
    void insert (bpfcp_t bpf);
    bpfcp_t getfile (const pfnm_t &nm) const;
    pbinding_t *to_binding (const pfnm_t &fn, const pfnm_t &rfn, 
			    bool rebind, bool toplev = false);
    pbinding_t *alloc (const pfnm_t &fn, phashp_t h, bool toplev = false);

    bindtab_t  bindings;                // expands filenames to hashes
    pfile_map_t files;                  // expands hashes to files
  };

  vec<str> rootfiles;    // all pub rootfiles
  bhash<str> rfbh;       // rootfile bhash
  bhash<str> crfbh;      // config rootfile bhash
  set_t *set;
  vec<pfnm_t> parsequeue;

  bool rebind;
  
  str homedir;
};

class pub_client_t : public pub_t 
{
public:
  pub_client_t () : pub_t (), nset (NULL) {}
  void explore (const pfnm_t &fn) const;
  static pub_client_t *alloc ();
  set_t *nset;
};

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

protected:
  void v_config_cb (const xpub_file_t &x) { cache_pubconf (x); }


private:
  bindtab_t bindings;
  ptr<xpub_file_t> pconf;
  qhash<phashp_t, ptr<xpub_file_t> > files;
  qhash<phashp_t, ptr<xpub_file_t> > recycle;
  mutable qhash<pfnm_t, bpfcp_t> my_cache;
};

class pub_parser_t : public pub_t 
{
public:
  pub_parser_t (bool exp = false) 
    : pub_t (), gvars (NULL), tag (NULL), 
    lasttag (NULL), 
    space_flag (false), xset_collect (false), 
    jaildir (""), jailed (false), jm (JAIL_NONE), exporter (exp) {}
  
  static pub_parser_t *alloc (bool exp = false);
  bool init (const str &fn);
  bpfcp_t parse_config (const str &fn, bool init = true);
  bpfcp_t parse (const pbinding_t *bnd, pfile_type_t t);
  void pwarn (const strbuf &b) 
  { if (bpf) warnx << bpf->loc () << ": "; warnx << b << "\n"; }

  void push_file (bpfmp_t f) { if (bpf) stack.push_back (bpf); bpf = f; }
  void pop_file ();
  void dump_stack ();

  void init_set () { xset.clear (); xset_collect = true; }
  void export_set (xpub_set_t *xs);
  void setprivs (str jd, str un, str gn);
  void setjail (str jd, bool permissive = false);
  str jail2real (const str &n) const;

  void send_up_the_river () { jailed = true; }
  void grant_conjugal_visit () { if (!(opts & P_DAEMON)) jailed = false; }
  bool behind_bars () const { return (opts & P_DAEMON) || jailed; }

  pbinding_t *to_binding (const pfnm_t &fn, set_t *s = NULL, 
			  bool toplev = false);
  void push_parr (ptr<parr_t> a);
  ptr<parr_t> pop_parr ();

  /* global parse variables */
  ptr<pstr_t> pstr;
  ptr<aarr_arg_t> aarr;
  ptr<arglist_t> arglist;
  ptr<parr_t> parr;
  concatable_str_t *str1;
  bpfmp_t bpf;
  gvars_t *gvars;
  pfile_func_t *func;
  pfile_html_tag_t *tag, *lasttag;

  bool space_flag;
  int last_tok;

  ptr<xpub_getfile_res_t> defconf;
private:
  bpfcp_t parse1 (const pbinding_t *bnd, pfile_sec_t *ss, pfile_type_t);
  bpfcp_t parse2 (const pbinding_t *bnd, pfile_sec_t *ss, pfile_type_t);
  str complete_fn (const str &in) const;
  vec<bpfmp_t> stack;
  vec<bpfcp_t> xset;
  bool xset_collect;
  str jaildir;
  bool jailed;
  jail_mode_t jm;
  vec<ptr<parr_t> > parr_stack;
  bool exporter;  // on if this pub_parser_t exports files via XDR

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
pub_t::cfg (const str &n, T *v) const
{
  str s;
  return (cfg (n, &s) && convertint (s, v));
}

const char * getenvval (const char *s);

#endif /* _LIBPUB_PUB_H */
