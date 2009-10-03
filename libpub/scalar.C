
#include "pscalar.h"
#include "parseopt.h"
#include "parr.h"

//-----------------------------------------------------------------------

static bool
my_convertint (const str &s, int64_t *out)
{
  bool ret = false;
  char *ep;
  errno = 0;
  if (!s || !s.len ()) {
    /* no-op -- empty string is not valid */
  } else {
    int64_t v = strtoll (s, &ep, 0);
    if (errno == ERANGE || errno == EINVAL) {
      /* no-op */
    } else if (ep && *ep == '\0') {
      *out = v;
      ret = true;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

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
      _int_cnv = (_s && my_convertint (_s, &_i)) ? CNV_OK : CNV_BAD;
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
  if (_s && _s.len ()) {
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
  int64_t i;
  bool ret = false;
  if (to_int64 (&i)) {
    ret = (i != 0);
  } else { 
    str s = to_str ();
    ret = ( s && s.len () > 0);
  }
  return ret;
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
  _uint_cnv = CNV_NONE;
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
scalar_obj_t::_p_t::set_i (int64_t i)
{
  _s = strbuf () << i;
  _i = i;
  _d = i;

  if (i >= 0) {
    _u = i;
    _uint_cnv = CNV_OK;
  } else {
    _uint_cnv = CNV_BAD;
  }

  _double_cnv = CNV_OK;
  _int_cnv = CNV_OK;
}

void
scalar_obj_t::_p_t::set_u (u_int64_t i)
{
  _s = strbuf () << i;
  _u = i;
  _d = i;
  
  if (i <= u_int64_t (INT64_MAX)) {
    _i = i;
    _int_cnv = CNV_OK;
  } else {
    _int_cnv = CNV_BAD;
  }

  _double_cnv = CNV_OK;
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
  _uint_cnv = _double_cnv = _int_cnv = CNV_NONE;
}

bool
convertuint (const str &s, u_int64_t *out)
{
  bool ret = false;
  char *ep;
  errno = 0;
  if (!s || !s.len ()) {
    /* no-op -- empty string is not valid */
  } else {
    u_int64_t v = strtoull (s, &ep, 0);
    if (errno == ERANGE || errno == EINVAL) {
      /* no-op */
    } else if (ep && *ep == '\0') {
      *out = v;
      ret = true;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

scalar_obj_t
scalar_obj_t::operator+ (const scalar_obj_t &s) const
{
  type_t me = natural_type ();
  type_t him = s.natural_type ();
  
  scalar_obj_t ret;

  if (me == _p_t::TYPE_STR || him == _p_t::TYPE_STR) {
    strbuf b;
    str s1 = to_str ();
    str s2 = s.to_str ();
    b << s1 << s2;
    ret.set (b);

  } else if (me == _p_t::TYPE_DOUBLE || him == _p_t::TYPE_DOUBLE) {
    double d1 = to_double ();
    double d2 = s.to_double ();
    double r = d1 + d2;
    ret.set (r);

  } else {
    int64_t i1, i2;
    u_int64_t u1, u2;

    if (to_int64 (&i1) && o.to_int64 (&i2)) {
      ret.set_i (i1 + i2);
    } else if (to_uint64 (&u1) && o.to_uint64 (&u2)) {
      ret.set_u (u1 + u2);
    } else if (to_int64 (&i1) && o.to_uint64 (&u2)) {
      ret.set_i (i1 + u2);
    } else if (to_uint64 (&u1) && o.to_int64 (&i2)) {
      ret.set_i (u1 + i2);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------

scalar_obj_t
scalar_obj_t::operator- (const scalar_obj_t &s) const
{
  type_t me = natural_type ();
  type_t him = s.natural_type ();
  
  scalar_obj_t ret;

  if (me == _p_t::TYPE_STR || him == _p_t::TYPE_STR) {
    /* can't subtract strings! */

  } else if (me == _p_t::TYPE_DOUBLE || him == _p_t::TYPE_DOUBLE) {
    double d1 = to_double ();
    double d2 = s.to_double ();
    double r = d1 - d2;
    ret.set (r);

  } else {
    int64_t i1, i2;
    u_int64_t u1, u2;
    if (to_int64 (&i1) && o.to_int64 (&i2)) {
      ret.set_i (i1 - i2);
    } else if (to_uint64 (&u1) && o.to_uint64 (&u2)) {
      ret.set_i (u1 - u2);
    } else if (to_int64 (&i1) && o.to_uint64 (&u2)) {
      ret.set_i (i1 - u2);
    } else if (to_uint64 (&u1) && o.to_int64 (&i2)) {
      ret.set_i (u1 - i2);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------

#define CMP(x,y) (x > y) ? 1 : ((x < y) ? -1 : 0)

int
scalar_obj_t::cmp (const scalar_obj_t &o) const
{
  type_t me = natural_type ();
  type_t him = o.natural_type ();
  int res;

  if (me == _p_t::TYPE_STR || them == _p_t::TYPE_STR) {
    str s1 = to_str ();
    str s2 = o.to_str ();
    res = s1.cmp (s2);

  } else if (me == _p_t::TYPE_DOUBLE || them == _p_t::TYPE_DOUBLE) {
    double d1 = to_double ();
    double d2 = o.to_double ();

    res = CMP (d1, d2);

  } else {
    int64_t i1, i2;
    u_int64_t u1, u2;

    if (to_uint64 (&u1) && o.to_uint64 (&u2)) {
      res = CMP (u1, u2);
    } else if (to_int64 (&i1) && o.to_int64 (&i2)) {
      res = CMP (i1, u2);
    } else if (to_uint64 (&u1) && o.to_int64 (&i2)) {
      res = CMP (1, -1);
    } else if (to_int64 (&i1) && o.to_uint64 (&u2)) {
      res = CMP (-1, 1);
    } else {
      res = 1;
    }
  }

  return res;
}

#undef CMP

//-----------------------------------------------------------------------

scalar_obj_t 
scalar_obj_t::operator* (const scalar_obj_t &o) const
{
  type_t me = natural_type ();
  type_t him = o.natural_type ();

  int64_t i1, i2;
  u_int64_t u1, u2;
  str s1, s2;
  scalar_obj_t out;

  if (me == _p_t::TYPE_STR && o.to_int64 (&i2) && i2 < 0x100) {
    strbuf b;
    for (int64_t i = 0; i < i2; i++) {
      b << s1;
    }
    out.set (b);

  } else if (me == _p_t::TYPE_STR || him == _p_t::TYPE_STR) {
    /* noop!!! */

  } else if (me == _p_t::TYPE_DOUBLE || him == _p_t::TYPE_DOUBLE) {
    double d1 = to_double ();
    double d2 = to_double ();
    out.set (d1 * d2);

  } else if (o1.to_int64 (&i1) && o2.to_int64 (&i2)) {
    out.set_i (i1*i2);
  } else if (o1.to_uint64 (&u1) && o2.to_uint64 (&u2)) {
    out.set_u (u1 * u2);
  } else if (o1.to_int64 (&i1) && o2.to_uint64 (&u2)) {
    out.set_i (u1 * u2);
  } else if (o1.to_uint64 (&u1) && o2.to_int64 (&i2)) {
    out.set_i (u1 * u2);
  } 

  return out;
}

//-----------------------------------------------------------------------


//=======================================================================
