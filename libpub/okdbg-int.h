
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
#include "rxx.h"

typedef enum {
  CHATTER = 0,
  ERROR = 1,
  FATAL_ERROR = 2,
} okdbg_lev_t;


#define OKDBG(x) \
  (okws_debug_flags & (x))

#define OKDBG2(x) \
  (okws_debug_flags & (OKWS_DEBUG_##x))

#define OKDBG3(o,l,s) \
do { if (OKDBG2(o)) { okdbg_warn (l,s); } } while (0)

#define OKDBG4(o,l,f,...) \
do { if (OKDBG2(o)) { okdbg_warn (l,f,__VA_ARGS__); } } while (0)

void okdbg_warn (okdbg_lev_t l, const str &s);

//
// provide __attribute__((format)) so that the compiler does sanity
// checking on the varargs lists, as it would for printf.
//
void okdbg_warn (okdbg_lev_t l, const char *fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

class okdbg_dumpable_t {
public:
  virtual ~okdbg_dumpable_t () {}
  virtual void okdbg_dump_vec (vec<str> *s) const = 0;
  virtual void okdbg_dump (okdbg_lev_t l = CHATTER) const 
  {
    vec<str> v;
    okdbg_dump_vec (&v);
    for (size_t i = 0; i < v.size () ; i++) {
      okdbg_warn (l, v[i]);
    }
  }

};

// environment variable name
#define OKWS_DEBUG_OPTIONS "OKWS_DEBUG_OPTIONS"

void set_debug_flags ();

class okdbg_t {
public:
  okdbg_t () {}
  virtual ~okdbg_t () {}
  virtual bool flag2bits (char c, int64_t *out) const;
  virtual const char *allflags () const;
  virtual const char *documentation () const;
  int main (int argc, char *argv[]);
  void usage ();
}; 


#endif /* _LIBPUB_DEBUG_H */
