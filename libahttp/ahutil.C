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


#include <time.h>
#include <stdio.h>
#include "ahutil.h"

static str timestr;
static time_t timestr_time = 0;

str
getdate (rfc_number_t rfc, time_t t1)
{
  bool def = (t1 == 0);
  if (def) {
    t1 = sfs_get_timenow ();
    if (t1 == timestr_time && timestr)
      return timestr;
    timestr_time = t1;
  }


  struct tm *t2 = gmtime (&t1);
  static char buf[100];
  size_t n = strftime (buf, 100, rfc_date_fmt (rfc), t2);

  str ret (buf, n);
  if (def) {
    timestr = ret;
  }

  return ret;
}

bool
mystrlcmp (const str &s, const char *b)
{
  const char *a = s.cstr ();
  const char *e = a + s.len ();
  while (a < e) 
    if (tolower (*a++) != tolower (*b++))
      return false;
  return true;
}

str
tolower_s (const str &s)
{
  mstr m (s.len ());
  char *cp;
  const char *cp2;
  for (cp = m.cstr (), cp2 = s.cstr (); *cp2; cp++, cp2++)
    *cp = tolower (*cp2);
  return m;
}

static void
stallcb1 (int sig, cbv c)
{
  warn << "Caught signal (" << sig << "), proceeding.\n";
  sigcb (sig, NULL);
  (*c) ();
}

void
stall (int sig, cbv c)
{
  warn << "Process PID=" << getpid () << " stalled, waiting "
       << "for signal " << sig << "\n";
  sigcb (sig, wrap (stallcb1, sig, c));
}

static void
stallcb2 (str fn, cbv c)
{
  if (access (fn.cstr (), R_OK) == 0) {
    warn << "Found file (" << fn << "), proceeding.\n";
    unlink (fn.cstr ());
    (*c) ();
  } else {
    delaycb (1, wrap (stallcb2, fn, c));
  }
}

void
stall (const str &fn, cbv c)
{
  warn << "Stalling (pid: " << getpid () << 
    "), checking for file: " << fn << "\n";
  stallcb2 (fn, c);
}

str
strip_newlines(str in) {

    mstr m(in.len());
    for (size_t i = 0; i < in.len(); i++) {
        if (in[i] == '\r' || in[i] == '\n') {
            m[i] = '\0';
            break;
        } else {
            m[i] = in[i];
        }
    }

    return m;
}
