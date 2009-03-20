
#include "pscalar.h"
#include "parseopt.h"
#include "parr.h"

scalar_obj_t::_p_t::_p_t () 
  : _double_cnv (CNV_NONE), _int_cnv (CNV_NONE), _uint_cnv (CNV_NONE) {}

scalar_obj_t::_p_t::_p_t (const str &s)
  : _s (s), _double_cnv (CNV_NONE), _int_cnv (CNV_NONE), _uint_cnv (CNV_NONE) {}

int64_t 
scalar_obj_t::_p_t::to_int64 () const
{
  if (_int_cnv == CNV_NONE) {

    if (_uint_cnv == CNV_OK && (_u <= u_int64_t (INT64_MAX))) {
      _i = _u;
      _int_cnv = CNV_OK;
    } else {
      _i = 0;
      _int_cnv = (_s && convertint (_s, &_i)) ? CNV_OK : CNV_BAD;
    }
  }
  return _i;
}

u_int64_t 
scalar_obj_t::_p_t::to_uint64 () const 
{
  if (_uint_cnv == CNV_NONE) {

    if (_int_cnv == CNV_OK && _i >= 0) {
      _u = _i;
      _uint_cnv = CNV_OK;
    } else {
      _u = 0;
      _uint_cnv = (_s && to_uint64 (&_u)) ? CNV_OK : CNV_BAD;
    }
  }
  return _u;
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

bool
scalar_obj_t::_p_t::to_uint64 (u_int64_t *out) const
{
  bool ret = false;
  if (_s) {
    ret = convertuint (_s, out);
  }
  return ret;
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

scalar_obj_t::scalar_obj_t () 
  : _p (New refcounted<_p_t> ()), _frozen (false) {}
scalar_obj_t::scalar_obj_t (const str &s) 
  : _p (New refcounted<_p_t> (s)), _frozen (false) {}

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

void
scalar_obj_t::_p_t::set (const str &s)
{
  _s = s;
  _double_cnv = CNV_NONE;
  _int_cnv = CNV_NONE;
}

void
scalar_obj_t::_p_t::set (double d)
{
#define BUFSZ 1024
  char buf[BUFSZ];
  size_t n = snprintf (buf, BUFSZ-1, "%g" , d);
  _s = buf;
  size_t lim = min<size_t> (BUFSZ -1, n);
  buf[lim] = '\0';
  _d = d;
  _double_cnv = CNV_OK;
  _int_cnv = CNV_BAD;
#undef BUFSZ
}

void
scalar_obj_t::_p_t::set (int64_t i)
{
  _s = strbuf () << i;
  _i = i;
  _d = i;
  _u = i;
  _double_cnv = CNV_OK;
  _int_cnv = CNV_OK;
}

void
scalar_obj_t::_p_t::set_u (u_int64_t i)
{
  _s = strbuf () << i;
  _i = i;
  _d = i;
  _u = i;
  _double_cnv = CNV_OK;
  _int_cnv = CNV_OK;
  _uint_cnv = CNV_OK;
}

void
scalar_obj_t::add (const char *c, size_t s)
{
  if (!_b) {
    _p->clear ();
    _b = New refcounted<strbuf> ();
  }
  _b->tosuio ()->copy (c, s);
}

void
scalar_obj_t::add (const str &s)
{
  add (s.cstr (), s.len ());
}

void
scalar_obj_t::freeze ()
{
  if (_b) {
    str s = *_b;
    _p->set (s);
    _b = NULL;
    _frozen = true;
  }
}

void
scalar_obj_t::_p_t::clear ()
{
  _s = NULL;
  _d = 0.0;
  _i = 0;
  _double_cnv = _int_cnv = CNV_NONE;
}

bool
convertuint (const str &s, u_int64_t *out)
{
  bool ret = false;
  char *ep;
  u_int64_t v = strtoull (s, &ep, 0);
  if (ep && *ep == '\0') {
    *out = v;
    ret = true;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
scalar_obj_t::operator== (const scalar_obj_t &o2) const
{
  int64_t i1, i2;
  u_int64_t u1, u2;
  double d1, d2;
  str s1, s2;
  bool ret;

  if (to_int64 (&i1) && o2.to_int64 (&i2)) { ret = (i1 == i2); }
  else if (to_uint64 (&u1) && o2.to_uint64 (&u2)) { ret = (u1 == u2); }
  else if (to_double (&d1) && o2.to_double (&d2)) { ret = (d1 == d2); }
  else {
    s1 = to_str ();
    s2 = o2.to_str ();
    if (s1 && s2) { ret = (s1 == s2); }
    else if (!s1 && !s2) { ret = true; }
    else { ret = false; }
  }
  return ret;
}

//-----------------------------------------------------------------------

//=======================================================================
