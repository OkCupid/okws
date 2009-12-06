// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Max Krohn (max@okcupid.com)
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

#ifndef _LIBAOK_OKERR_H
#define _LIBAOK_OKERR_H 1

#include "xpub.h"
#include "okprot.h"
#include "puberr.h"

class ok_res_t : public pub_res_t {
public:
  ok_res_t () : pub_res_t () {}

  ok_res_t (pub_res_t *r) 
  { 
    status = r->status; 
    str t = r->errbuf; 
    errbuf << t;
  }

  ok_xstatus_t to_xdr ()
  {
    ok_xstatus_t o;
    if (status) {
      o.set_status (OK_STATUS_OK);
    } else {
      o.set_status (OK_STATUS_ERR);
      *o.error = str (errbuf);
    }
    return o;
  }

  void add (const ok_xstatus_t &s)
  { if (s.status != OK_STATUS_OK) pub_res_t::add (*s.error); }

};

typedef callback<void, ptr<ok_res_t> >::ref okrescb;

#endif /* LIBAOK_OKERR_H */

