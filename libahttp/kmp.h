
// -*-c++-*-
/* $Id$ */

//
// Knutt-Morris-Pratt String Matching Algorithm
//   as presented in CLR (Second Edition) Section 32.4
//  
#include "async.h"
#include <string.h>

class kmp_matcher_t {
public:
  kmp_matcher_t (const str &s) { preproc (s.cstr (), s.len ()); }
  kmp_matcher_t (const char *c, u_int l) { preproc (c, l); }
  kmp_matcher_t (const char *c) { preproc (c, strlen (c)); }
  ~kmp_matcher_t () { delete [] pi; delete [] P; }

  inline bool match (char c)
  {
    while (q > 0 && P[q] != c) q = pi[q-1];
    if (P[q] == c) q++;
    if (q == len) { q = pi[q-1]; return true; }
    else { return false; }
  }

private:
  void preproc (const char *c, u_int l);
  u_int *pi;
  char *P;
  u_int len;
  u_int q;

};
