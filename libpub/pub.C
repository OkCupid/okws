/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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

#include <sys/param.h>
#include <stdlib.h>
#include "pub_parse.h"
#include "pub.h"
#include "rxx.h"
#include "okdbg.h"

/* global publishing objects */
pub_parser_t *global_parser;

char dwarnbuf[1024];

bound_pfile_t::~bound_pfile_t ()
{
  if (file && delfile)
    delete file;
}

str
pub_parser_t::jail2real (const str &n) const
{
  str ret;
  if (!n || !behind_bars ())
    ret = n;
  else {
    switch (jm) {
    case JAIL_NONE:
    case JAIL_REAL:
      ret = n;
      break;
    case JAIL_VIRTUAL:
      {
	const char *cp = n.cstr ();
	while (*cp++ == '/') ;
	if (*cp) cp--;
	ret = strbuf (jaildir) << "/" << cp;
	break;
      }
    default:
      break;
    }
  }
  if (be_verbose ()) 
    warn << "jail2real: " << n << " --> " << ret << "\n";
  return ret;
}

pbinding_t *
pub_parser_t::to_binding (const pfnm_t &fn)
{
  pfnm_t fn2 = complete_fn (fn);
  if (!fn2) return NULL;
  return New pbinding_t (fn2, NULL);
}

void
pub_parser_t::dump_stack ()
{
  size_t i = 0;
  for (; i < stack.size (); i++) 
    warnx << "\t" << i << ": " << stack[i]->loc ();
  if (bpf)
    warnx << "\t" << i << ": " << bpf->loc ();
}

static rxx dds ("\\.\\./");
pfnm_t 
pub_parser_t::complete_fn (const pfnm_t &fn) const
{
  if (!fn)
    return NULL;
  str ret;
  const char *s1 = fn.cstr ();
  if (dds.search (fn) && (jm == JAIL_VIRTUAL || jm == JAIL_REAL)) {
    ret = NULL;
  } else {
    if (*s1 == '/' || !bpf) {
      ret = fn;
    } else {
      const str &fn2 = bpf->filename ();
      const char *s2 = fn2.cstr ();
      const char *s3 = s2 + fn2.len () - 1;
      while (s3 > s2 && *s3 != '/') s3--;
      str pdir = (*s3 == '/') ? str (s2, s3 - s2 + 1) : str ("/");
      ret = strbuf (pdir) << s1;
    }
  } 
  if (be_verbose ())
    warn << "complete_fn: " << fn << " --> " << (ret ? ret : str ("(null)"))
	 << "\n"; // debug
  return ret;
}

pub_parser_t *
pub_parser_t::alloc (bool exp)
{
  return New pub_parser_t (exp);
}

void
pub_parser_t::pop_file ()
{
  if (stack.size ()) {
    bpf = stack.pop_back ();
  } else {
    bpf = NULL;
  }
}

void
pub_parser_t::setjail (str jd, bool permissive)
{
  if (!jd && !(jd = cfg ("JailDir"))) jd = ".";
  if (jd != "." && !isdir (jd)) 
    fatal << jd << ": cannot access jail directory\n";

  if (permissive) {
    jm = JAIL_PERMISSIVE;
  } else if (getuid ()) {
    jm = JAIL_VIRTUAL;
  } else {
    jm = JAIL_REAL;
    if (jd != "." && chroot (jd))
      fatal << jd << ": cannot chroot\n";
  }
  if (jd != "." && be_verbose ())
    warn << "Top (Jail) Directory is: " << jd << "\n";
  jaildir = jd;
}

void
pub_parser_t::setprivs (str jd, str un, str gn)
{
  int uid = 0, gid = 0;
  if (!getuid ()) {
    if (!un) {
      un = ok_pubd_uname;
      warn << "No pub username given; defaulting to '" <<  un << "'\n";
    }
    if (!gn) {
      gn = ok_pubd_gname;
      warn << "No pub groupname given; defaulting to '" <<  gn << "'\n";
    }
    
    // need to call this before chroot'ing! otherwise, 
    // we can't open /etc/passwd
    if ((uid = uname2uid (un)) < 0) fatal << un << ": no such user\n";
    if ((gid = gname2gid (gn)) < 0) fatal << gn << ": no such group\n";
  }

  setjail (jd, false);
  lock_in_jail ();

  if (!getuid ()) {
    setgid (gid);
    if (uid) setuid (uid);
  }
}

str 
pub_config_iface_t::cfg (const str &n, bool allownull) const
{
  ref<pvar_t> v (New refcounted<pvar_t> (n));
  return v->eval (get_env (), EVAL_INTERNAL, allownull);
}

bool
pub_config_iface_t::cfg (const str &n, const pval_t **v) const
{
  return ((*v = get_env ()->lookup (n, false)));
}

bool
pub_config_iface_t::cfg (const str &n, str *s, bool allownull) const
{
  return ((*s = cfg (n, allownull)));
}

pfnm_t 
pub_parser_t::apply_homedir (const pfnm_t &n) const
{
  return (!n || n.len () <= 0 || !homedir || n[0] == '/') ?
    n : str (strbuf (homedir) << "/" << n);
}

void
pub_parser_t::push_parr (ptr<parr_t> p)
{
  if (parr)
    parr_stack.push_back (parr);
  parr = p;
}

ptr<parr_t>
pub_parser_t::pop_parr ()
{
  ptr<parr_t> ret = parr;
  if (parr_stack.size ())
    parr = parr_stack.pop_back ();
  return ret;
}

const char *
getenvval (const char *s)
{
  const char *p;
  for (p = s; *p && *p != '='; p++);
  while (isspace (*p++)) ;
  return p;
}

// new parsing routine for pub2
ptr<bound_pfile2_t>
pub_parser_t::pub2_parse(ptr<pbinding_t> bnd, int opts, pubstat_t *err,
			 str *err_msg)
{
  bpfmp_t r;
  const str &fn = bnd->filename ();
  bool wss = opts & P_WSS;
  pfile_type_t t = wss ? PFILE_TYPE_WH : PFILE_TYPE_H;
  pfile_sec_t *ss = New pfile_html_sec_t (0);
  ptr<bound_pfile2_t> ret = 
    New refcounted<bound_pfile2_t> (bnd, jail2real (fn), t, opts);
  pfile_t *pf = ret->file ();
  *err = PUBSTAT_OK;
  int wss_prev = yywss;
  yywss = wss ? 1 : 0;

  if (opts & P_NOPARSE) {
    str d = ret->bpf ()->read ();
    if (!d) {
      PWARN (fn << ": cannot read raw file");
      delete ss;
      ret = NULL;
    } else {
      ss->add (New pfile_raw_el_t (d));
      pf->add_section (ss);
    }
  } else {
    int old_opts = get_opts ();
    int new_opts = old_opts;
    set_opts (new_opts);
    
    r = ret->nonconst_bpf ();
    if (!r->open ()) {
      PWARN (fn << ": failed to open file");
      delete ss;
      ret = NULL;
    } else {
      pub_parser_t *old_parser = global_parser;
      
      global_parser = this;
      push_file (r);
      pf->push_section (ss);
      pf->lex_activate (t);
      yyparse ();
      pf->add_section (ss);
      pop_file ();
      r->close ();
      if (pf->err != PUBSTAT_OK) {
	PWARN (fn << ": parse failed");
	*err = pf->err;
	*err_msg = pf->err_msg;
	ret = NULL;
      }
      global_parser = old_parser;
    }
    set_opts (old_opts);
    yywss = wss_prev;
  }
  return ret;
}


void 
pub_parser_t::pwarn (const strbuf &b) 
{ 
  strbuf msg;
  if (bpf) 
    msg << bpf->loc () << ": "; 
  msg << b;
  if (bpf && bpf->file)
    bpf->file->err_msg = msg;
  warn << msg << "\n";
}
