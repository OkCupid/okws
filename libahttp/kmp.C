
#include "kmp.h"


void
kmp_matcher_t::preproc (const char *pattern, u_int l)
{
  len = l;
  pi = new u_int[len];
  P = new char[len];
  memcpy (P, pattern, len);
  memset (pi, 0, len * sizeof (u_int));
  u_int k = 0;
  for (u_int q_tmp = 1; q_tmp < len; q_tmp++) {
    while (k > 0  && P[k] != P[q_tmp]) 
      k = pi[k-1];
    if (P[k] == P[q_tmp])
      k++;
    pi[q_tmp] = k;
  }

  q = 0;
}
