
/* $Id$ */

#include "okd.h"
#include "okprot.h"

okd_mgrsrv_t::okd_mgrsrv_t (ptr<axprt_stream> xx, okd_t *p) : 
  x (xx), myokd (p)
{
  srv = asrv::alloc (x, okmgr_program_1, wrap (this, &okd_mgrsrv_t::dispatch));
}

void
okd_mgrsrv_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    delete this;
    return;
  }
  u_int p = sbp->proc ();
  switch (p) {
  case OKMGR_NULL:
    sbp->reply (NULL);
    break;
  case OKMGR_REPUB:
    repub (sbp);
    break;
  case OKMGR_RELAUNCH:
    relaunch (sbp);
    break;
  case OKMGR_TURNLOG:
    turnlog (sbp);
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

static void 
replystatus (svccb *s, ptr<ok_res_t> res) 
{ 
  s->replyref (res->to_xdr ()); 
}

void
okd_mgrsrv_t::relaunch (svccb *sbp)
{
  ok_progs_t *p = sbp->template getarg<ok_progs_t> ();
  myokd->relaunch (*p, wrap (replystatus, sbp));
}

void
okd_mgrsrv_t::repub (svccb *sbp)
{
  xpub_fnset_t *r = sbp->template getarg<xpub_fnset_t> ();
  myokd->repub (*r, wrap (replystatus, sbp));
}

void
okd_mgrsrv_t::turnlog (svccb *sbp)
{
  myokd->turnlog (wrap (replystatus, sbp));
}

