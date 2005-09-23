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
    repub (sbp, 1);  // version # == 1
    break;
  case OKMGR_REPUB2:
    repub (sbp, 2);  // version # == 2
    break;
  case OKMGR_RELAUNCH:
    relaunch (sbp);
    break;
  case OKMGR_TURNLOG:
    turnlog (sbp);
    break;
  case OKMGR_CUSTOM_1:
    myokd->custom1_in (sbp);
    break;
  case OKMGR_CUSTOM_2:
    myokd->custom2_in (sbp);
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

void
okd_mgrsrv_t::relaunch (svccb *sbp)
{
  ok_progs_t *p = sbp->Xtmpl getarg<ok_progs_t> ();
  myokd->relaunch (*p, wrap (replystatus, sbp));
}

void
okd_mgrsrv_t::repub (svccb *sbp, int v)
{
  xpub_fnset_t *r = sbp->Xtmpl getarg<xpub_fnset_t> ();
  if (v == 1) 
    myokd->repub (*r, wrap (replystatus, sbp));
  else
    myokd->repub2 (*r, wrap (replystatus, sbp));
}

void
okd_mgrsrv_t::turnlog (svccb *sbp)
{
  myokd->turnlog (wrap (replystatus, sbp));
}

