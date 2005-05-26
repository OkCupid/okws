
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

#ifndef _LIBPUB_DEBUG_H
#define _LIBPUB_DEBUG_H 1

#include "okconst.h"

typedef enum {
  CHATTER = 0,
  ERROR = 1,
  FATAL_ERROR = 2,
} okdbg_lev_t;


#define OKDBG(x) \
  (okws_debug_flags & (x))

#define OKDBG2(x) \
  (okws_debug_flags & (OKWS_DEBUG_##x))

inline void
okdbg_warn (okdbg_lev_t l, const str &s)
{
  switch (l) {
  case CHATTER:
    warn << "++ ";
    break;
  case ERROR:
    warn << "** ";
    break;
  case FATAL_ERROR:
    warn << "XX ";
    break;
  default:
    warn << "";
    break;
  }
  warnx << s;
  if (s[s.len () - 1] != '\n')
    warnx << "\n";
}


class okdbg_dumpable_t {
public:
  virtual void okdbg_dump_vec (vec<str> *s) const = 0;
  virtual void okdbg_dump (okdbg_lev_t l = CHATTER) const 
  {
    vec<str> v;
    okdbg_dump_vec (&v);
    for (u_int i = 0; i < v.size () ; i++) {
      okdbg_warn (l, v[i]);
    }
  }

};

// environment variable name
#define OKWS_DEBUG_OPTIONS "OKWS_DEBUG_OPTIONS"

#endif /* _LIBPUB_DEBUG_H */
