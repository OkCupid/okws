
#include "kmp.h"

void
kmp_matcher_t::preproc ()
{
  memset (_pi, 0, _len * sizeof (u_int));
  u_int k = 0;
  for (u_int q_tmp = 1; q_tmp < _len; q_tmp++) {
    while (k > 0  && _P[k] != _P[q_tmp]) 
      k = _pi[k-1];
    if (_P[k] == _P[q_tmp])
      k++;
    _pi[q_tmp] = k;
  }
}
