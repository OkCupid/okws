// -*-c++-*-
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

#ifndef _LIBAHTTP_AHUTIL
#define _LIBAHTTP_AHUTIL 1

#include "async.h"
#include "httpconst.h"

typedef u_char htpv_t;
typedef u_int16_t okws1_port_t;

str getdate (rfc_number_t rfc = RFC_1123, time_t tm = 0);
bool mystrlcmp (const str &s, const char *b);
str tolower_s (const str &in);
str strip_newlines(str in);

inline 
int char_at (const str &s, int i)
{ 
  if (!s) return -1;
  i = (i < 0) ? s.len () + i : i;
  return (i < 0 || i >= int (s.len ())) ? -1 : s[i];
}

void stall (int sig, cbv c);
void stall (const str &fn, cbv c);

class cbbun_t : public virtual refcount {
public:
  cbbun_t (u_int i, cbv c) : cnt (i), cb (c), called (false) 
  { 
    if (!i) {
      called = true;
      (*c) (); 
    }
  }
   
  void dec (ptr<cbbun_t> d = NULL)
  {
    if (!called && !--cnt) {
      called = true;
      (*cb) ();
    }
  }

  cbv::ptr bwrap ()
  {
    return wrap (this, &cbbun_t::dec, mkref (this)); 
  }

  static ptr<cbbun_t> alloc (u_int i, cbv c) 
  { return New refcounted<cbbun_t> (i, c); }

private:
  u_int cnt;
  cbv cb;
  bool called;
};

#endif /* _LIBAHTTP_AHUTIL */
