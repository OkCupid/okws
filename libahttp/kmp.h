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

//
// Knuth-Morris-Pratt String Matching Algorithm
//   as presented in CLR (Second Edition) Section 32.4
//  
// trying a different coding style here on for size; using
// "_X" for member variables, and X (), for accessor methods.
// i think i might prefer this method...
//
#ifndef _LIBAHTTP_KMP_H
#define _LIBAHTTP_KMP_H

#include "async.h"
#include <string.h>
#include "ahutil.h"

class kmp_matcher_t {
public:
  kmp_matcher_t (const str &s, bool case_insensitive = true) 
    : _ci (case_insensitive),
      _pattern (_ci ? str (tolower_s (s)) : s), 
      _PP (_pattern.cstr ()), _len (_pattern.len ()),
      _pi (new u_int[_len]), _q (0) 
  { preproc (); }

  ~kmp_matcher_t () { delete [] _pi; }

  void reset () { _q = 0; }

  inline bool match (char c)
  {
    if (_ci) c = tolower (c);
    while (_q > 0 && _PP[_q] != c) _q = _pi[_q-1];
    if (_PP[_q] == c) _q++;
    if (_q == _len) { _q = _pi[_q-1]; return true; }
    else { return false; }
  }

  inline u_int len () const { return _len; }
  str pattern () const { return _pattern; }

private:
  void preproc ();

  const bool _ci;
  const str _pattern;
  const char *const _PP;
  const u_int _len;
  u_int *_pi;
  u_int _q;

};

#endif /* _LIBAHTTP_KMP_H */
