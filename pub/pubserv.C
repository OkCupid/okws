/* $Id$ */

#include "pubd.h"
#include "puberr.h"

pubserv_t::pubserv_t (ptr<axprt_stream> xx, bool p)
  : x (xx), primary (p)
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
  xpub_getfile_res_t x;
  bpfcp_t fp;
  if (pub->defconf) {
    x.set_status (XPUB_STATUS_OK);
    pub->defconf->file->to_xdr (x.file);
  } else {
    x.set_status (XPUB_STATUS_NOENT);
  }
  sbp->replyref (x);
}

void
pubserv_t::getfile (svccb *sbp)
{
  str err;
  xpub_getfile_res_t res (XPUB_STATUS_OK);
  xpubhash_t *r = sbp->template getarg<xpubhash_t> ();
  phashp_t hsh = phash_t::alloc (*r);
  pfile_t *f = parser->getfile (hsh);
  if (!f) res.set_status (XPUB_STATUS_NOENT);
  else f->to_xdr (res.file);
  sbp->replyref (res);
}

void
pubserv_t::lookup (svccb *sbp)
{
  xpub_fn_t *x = sbp->template getarg<xpub_fn_t> ();
  pbinding_t *bnd = parser->to_binding (*x);
  xpub_lookup_res_t res (XPUB_STATUS_OK);
  bpfcp_t bpf;
  if (!bnd) {
    res.set_status (XPUB_STATUS_NOENT);
  } else if ((bpf = parser->parse (bnd, PFILE_TYPE_H))) {
    bnd->hash ()->to_xdr (res.hash);
  } else {
    str err = strbuf ("Parse error in file: ") << *x << "\n";
    res.set_status (XPUB_STATUS_ERR);
    *res.error = err;
  }
  sbp->replyref (res);
}

void
pubserv_t::pubfiles (svccb *sbp)
{
  xpub_fnset_t *f =  sbp->template getarg<xpub_fnset_t> ();
  u_int lim = f->files.size ();
  pub_res_t res;
  xpub_result_t pres;
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
  parser->export_set (&pres.set);
  parser->rebind = rebind_orig;
  res.to_xdr (&pres.status);
  sbp->replyref (pres);
}
