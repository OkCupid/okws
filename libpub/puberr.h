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

#ifndef _LIBPUB_PUBERR_H
#define _LIBPUB_PUBERR_H 1

#include "xpub.h"
#include "err.h"

class pub_res_t {
public:
  pub_res_t () : status (true) {}
  void add (bool b) { status = false; }

  operator bool () const { return status; }
  str to_str () const { return errbuf; }
  void add (const strbuf &b) { add (str (b)); }
  void add (str s) 
  { 
    status = false; 
    errbuf << s;
    if (s[s.len () - 1] != '\n') errbuf << "\n";
  }
  
  void add (const xpub_lookup_res_t &x, const str &fn);

  template<class C> void add_xdr_res (const C &x, const str &fn);

  void to_xdr (xpub_status_t *x)
  {
    if (status) {
      x->set_status (XPUB_STATUS_OK);
    } else {
      x->set_status (XPUB_STATUS_ERR);
      *x->error = str (errbuf);
    }
  }
  
  void add (const xpub_status_t &s)
  { if (s.status != XPUB_STATUS_OK) add (*s.error); }

  void cluck () const { if (errbuf.iovcnt ()) warn << errbuf; }

  bool status;
  strbuf errbuf;
};

template<class C> void
pub_res_t::add_xdr_res (const C &x, const str &nm)
{
  switch (x.status) {
  case XPUB_STATUS_NOENT:
    status = false;
    errbuf << nm << ": file not found";
    break;
  case XPUB_STATUS_ERR:
    status = false;
    errbuf << *x.error;
    break;
  default:
    break;
  }
}

inline const strbuf &
operator<< (pub_res_t &res, const strbuf &b)
{
  warn << b << "\n";
  res.add (b);
  return (b);
}

typedef callback<void, ptr<pub_res_t> >::ref pubrescb;

#endif /* LIBPUB_PUBERR_H */

