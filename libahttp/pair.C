
#include "pair.h"

void
pair_t::dump1 () const
{
  warnx << key;
  if (vals.size () == 0) {
    warnx << "\n";
    return;
  }
  warnx << " --> ";
  if (vals[0])
    warnx << vals[0];

  warnx << "\n";
}

vec<int64_t> *
pair_t::to_int () const
{
  vec<int64_t> *ret = NULL;
  switch (is) {
  case IV_ST_NONE:
    {
      size_t s = vals.size ();
      int64_t tmp;
      ivals.setsize (s);
      for (u_int i = 0; i < s; i++) {
	if (!convertint (vals[i], &ivals[i])) {
	  is = IV_ST_FAIL;
	  break;
	}
      }
      if (is != IV_ST_FAIL) {
	is = IV_ST_OK;
	ret = &ivals;
      }
      break;
    }
  case IV_ST_OK:
    ret = &ivals;
    break;
  case IV_ST_FAIL:
  default:
    break;
  }
  return ret;
}

bool
pair_t::to_int (int64_t *v) const
{
  vec<int64_t> *p = to_int ();
  if (p && p->size () > 0) {
    *v = (*p)[0];
    return true;
  } 
  return false;
}
