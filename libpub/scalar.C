
#include "pscalar.h"
#include "parseopt.h"

scalar_obj_t::_p_t::_p_t () 
  : _double_cnv (CNV_NONE), _int_cnv (CNV_NONE) {}

scalar_obj_t::_p_t::_p_t (const str &s)
  : _s (s), _double_cnv (CNV_NONE), _int_cnv (CNV_NONE) {}

int64_t 
scalar_obj_t::_p_t::to_int64 () const
{
  if (_int_cnv == CNV_NONE) {
    _i = 0;
    _int_cnv = (_s && convertint (_s, &_i)) ? CNV_OK : CNV_BAD;
  }
  return _i;
}

bool
scalar_obj_t::_p_t::to_int64 (int64_t *out) const
{
  int64_t t = to_int64 ();
  if (_int_cnv == CNV_OK) {
    *out = t;
    return true;
  }
  return false;
}

int
scalar_obj_t::_p_t::to_int () const
{
  int64_t i = to_int64 ();
  return i;
}

bool
convertdouble (const str &x, double *dp)
{
  const char *bp = x.cstr ();
  char *ep;

  double d = strtod (bp, &ep);
  if (*ep) return false;
  *dp = d;
  return true;
}

double
scalar_obj_t::_p_t::to_double () const
{
  if (_double_cnv == CNV_NONE) {
    _d = 0.0;
    _double_cnv = (_s && convertdouble (_s, &_d)) ? CNV_OK : CNV_BAD;
  }
  return _d;
}

bool
scalar_obj_t::_p_t::to_double (double *out) const
{
  double d = to_double ();
  if (_double_cnv == CNV_OK) {
    *out = d;
    return true;
  }
  return false;
}


bool
scalar_obj_t::_p_t::to_bool () const
{
  int64_t i = to_int ();
  return i != 0;
}

str
scalar_obj_t::_p_t::to_str () const
{
  if (!_s) return "";
  return _s;
}

scalar_obj_t::scalar_obj_t () : _p (New refcounted<_p_t> ()) {}
scalar_obj_t::scalar_obj_t (const str &s) : _p (New refcounted<_p_t> (s)) {}

str
scalar_obj_t::trim () const
{
  str s = to_str ();
  if (s.len () == 0) return s;
  const char *bp = s.cstr ();
  size_t len = s.len ();
  const char *ep = bp + len;
  size_t i = 0;

  for ( i = 0; i < len && isspace (bp[i]); i++);
  if (i == len) return "";
  bp += i;

  for ( ep --; isspace (*ep) && ep > bp; ep --) ;
  ep ++;

  return str (bp, ep - bp);
}


//-----------------------------------------------------------------------
//=======================================================================
