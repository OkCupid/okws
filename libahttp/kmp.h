
// -*-c++-*-
/* $Id$ */

//
// Knutt-Morris-Pratt String Matching Algorithm
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
      _pattern (_ci ? tolower (s) : s), 
      _P (_pattern.cstr ()), _len (_pattern.len ()),
      _pi (new u_int[_len]), _q (0) 
  { preproc (); }

  ~kmp_matcher_t () { delete [] _pi; }

  void reset () { _q = 0; }

  inline bool match (char c)
  {
    if (_ci) c = tolower (c);
    while (_q > 0 && _P[_q] != c) _q = _pi[_q-1];
    if (_P[_q] == c) _q++;
    if (_q == _len) { _q = _pi[_q-1]; return true; }
    else { return false; }
  }

  inline u_int len () const { return _len; }

private:
  void preproc ();

  const bool _ci;
  const str _pattern;
  const char *const _P;
  const u_int _len;
  u_int *_pi;
  u_int _q;

};

#endif /* _LIBAHTTP_KMP_H */
