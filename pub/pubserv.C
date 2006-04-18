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
#include "pubd.h"
#include "puberr.h"

pubserv_t::pubserv_t (ptr<axprt_stream> xx, bool p)
  : x (xx), primary (p), this_cookie (0)
{
  srv = asrv::alloc (x, pub_program_1, wrap (this, &pubserv_t::dispatch));
}

void
pubserv_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    if (primary) {
      warn << "EOF received; shutting down.\n";
      exit (0);
    } else {
      delete this;
      return;
    }
  }
  u_int p = sbp->proc ();
  switch (p) {
  case PUB_NULL:
    sbp->reply (NULL);
    break;
  case PUB_FILES:
    pubfiles (sbp);
    break;
  case PUB_GETFILE:
    getfile (sbp);
    break;
  case PUB_CONFIG:
    config (sbp);
    break;
  case PUB_LOOKUP:
    lookup (sbp);
    break;
  case PUB_FILES2:
    pubfiles2 (sbp);
    break;
  case PUB_FILES2_GETFILE:
    pubfiles2_getfile (sbp);
    break;
  case PUB_FILES2_CLOSE:
    pubfiles2_close (sbp);
    break;
  case PUB_KILL:
    warn << "Caught kill RPC; shutting down\n";
    exit (0);
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
  return;
}

void
pubserv_t::config (svccb *sbp)
{
  if (parser->defconf) {
    sbp->reply (parser->defconf);
  } else {
    xpub_getfile_res_t x;
    x.set_status (XPUB_STATUS_NOENT);
    sbp->replyref (x);
  }
}

void
pubserv_t::getfile (svccb *sbp)
{
  str err;
  xpub_getfile_res_t res (XPUB_STATUS_OK);
  xpubhash_t *r = sbp->Xtmpl getarg<xpubhash_t> ();
  phashp_t hsh = phash_t::alloc (*r);
  pfile_t *f = parser->getfile (hsh);
  if (!f) res.set_status (XPUB_STATUS_NOENT);
  else f->to_xdr (res.file);
  sbp->replyref (res);
}

void
pubserv_t::lookup (svccb *sbp)
{
  xpub_fn_t *x = sbp->Xtmpl getarg<xpub_fn_t> ();
  pbinding_t *bnd = parser->to_binding (*x);
  xpub_lookup_res_t res (XPUB_STATUS_OK);
  bpfcp_t bpf;
  if (!bnd) {
    warn << "Cannot find file: " << *x << "\n";
    res.set_status (XPUB_STATUS_NOENT);
  } else if ((bpf = parser->parse (bnd, PFILE_TYPE_H))) {
    bnd->hash ()->to_xdr (res.hash);
  } else {
    warn << "Parse error in file: " << *x << "\n";
    str err = strbuf ("Parse error in file: ") << *x << "\n";
    res.set_status (XPUB_STATUS_ERR);
    *res.error = err;
  }
  sbp->replyref (res);
}

void
pubserv_t::pubfiles2 (svccb *sbp)
{
  xpub_cookie_t cookie = next_session_cookie ();
  ptr<xpub_result_t> pres = New refcounted<xpub_result_t> ();
  xpub_result2_t pres2;
  pubfiles (sbp, pres);

  pres2.status = pres->status;
  if (pres->status.status == XPUB_STATUS_OK) {
    sessions.insert (cookie, pres);
    pres2.set.bindings = pres->set.bindings;
    pres2.set.nfiles = pres->set.files.size ();
    pres2.set.cookie = cookie;
  }
  sbp->replyref (pres2);
}

void
pubserv_t::pubfiles2_close (svccb *sbp)
{
  xpub_cookie_t *cookie = sbp->Xtmpl getarg<xpub_cookie_t> ();
  xpub_status_typ_t t = XPUB_STATUS_OK;
  if (sessions[*cookie]) {
    sessions.remove (*cookie);
  } else {
    t = XPUB_STATUS_NOENT;
  }
  sbp->replyref (&t);
}


void
pubserv_t::pubfiles2_getfile (svccb *sbp)
{
  xpub_getfile_res_t res (XPUB_STATUS_OK);
  xpub_files2_getfile_arg_t *arg = 
    sbp->Xtmpl getarg<xpub_files2_getfile_arg_t> ();
  ptr<xpub_result_t> *pres = sessions[arg->cookie];
  if (!pres) {
    res.set_status (XPUB_STATUS_NOENT);
  } else if ((*pres)->set.files.size () <= arg->fileno) {
    res.set_status (XPUB_STATUS_OOB);
  } else {
    *res.file = (*pres)->set.files[arg->fileno];
  }
  sbp->replyref (res);
}

void
pubserv_t::pubfiles (svccb *sbp)
{
  xpub_result_t pres;
  pubfiles (sbp, &pres);
  sbp->replyref (pres);
}

void
pubserv_t::pubfiles (svccb *sbp, xpub_result_t *pres)
{
  xpub_fnset_t *f =  sbp->Xtmpl getarg<xpub_fnset_t> ();
  u_int lim = f->files.size ();
  pub_res_t res;
  parser->init_set ();
  bool rebind_orig = parser->rebind;
  parser->rebind = f->rebind;

  for (u_int i = 0; i < lim; i++) {
    str fn = f->files[i];
    pbinding_t *bnd = parser->to_binding (fn);
    if (!bnd) 
      res << (strbuf ("Cannot find file for parsing: ")  << fn);
    else if (!parser->parse (bnd, PFILE_TYPE_H))
      res << (strbuf ("Parse error in file: ") << fn);
  }
  parser->export_set (&pres->set);
  parser->rebind = rebind_orig;
  res.to_xdr (&pres->status);
}
