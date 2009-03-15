
#include "pub.h"
#include "pscalar.h"
#include "parseopt.h"
#include "okformat.h"


ptr<pub_range_t>
pub_range_t::alloc (const str &pat, const str &o, str *ep)
{
  static rxx ix ("\\[\\s*((-|0x)?[0-9a-fA-F]+)\\s*-"
		 "\\s*((-|0x)?[0-9a-fA-F]+)\\s*\\]");
  static rxx ux ("\\[\\s*((0x)?[0-9a-fA-F]+)[uU]\\s*-"
		 "\\s*((0x)?[0-9a-fA-F]+)[uU]\\s*\\s*\\]");
  static rxx dx ("\\[\\s*(-?[0-9.]+)\\s*-\\s*(-?[0-9.]+)\\s*\\]");

  ptr<pub_range_t> ret;
  str e;

  if (ux.match (pat)) {
    u_int64_t lo (0), hi (0);
    if (!convertuint (ux[1], &lo))  {
      e = strbuf ("Cannot convert low end of range (") << ux[1] << ")";
    } else if (!convertuint (ux[3], &hi)) {
      e = strbuf ("Cannot convert hi end of range (") << ux[3] << ")";
    } else if (lo > hi) {
      e = strbuf ("range impossible to satisfy (lo > hi)");
    } else {
      ret = New refcounted<pub_urange_t> (lo, hi);
    }
  } else if (ix.match (pat)) {
    int64_t lo (0), hi (0);
    if (!convertint (ix[1], &lo)) {
      e = strbuf ("Cannot convert low end of range (") << ix[1] << ")";
    } else if (!convertint (ix[3], &hi)) {
      e = strbuf ("Cannot convert hi end of range (") << ix[3] << ")";
    } else if (lo > hi) {
      e = strbuf ("range impossible to satisfy (lo > hi)");
    } else {
      ret = New refcounted<pub_irange_t> (lo, hi);
    }
  } else if (dx.match (pat)) {
    double lo (0.0), hi (0.0);
    if (!convertdouble(dx[1], &lo)) {
      e = strbuf ("Cannot convert low end of float-range (") << dx[1] << ")";
    } else if (!convertdouble (dx[2], &hi)) {
      e = strbuf ("Cannot convert hi end of float-range (") << dx[3] << ")";
    } else if (lo > hi) {
      e = strbuf ("range impossible to satisfy (lo > hi)");
    } else {
      ret = New refcounted<pub_drange_t> (lo, hi);
    }
  } else {
    e = "Unrecognized range given";
  }
  if (e && ep) *ep = e;
  return ret;
}

str
pub_irange_t::to_str () const
{
  strbuf b ("Irange: [%" PRId64 ", %" PRId64 "]", _low, _hi);
  return b;
}

bool
pub_irange_t::match (scalar_obj_t so) 
{
  int64_t i;
  if (!so.to_int64 (&i)) return false;
  return (i >= _low && i <= _hi);

}

str
pub_drange_t::to_str () const
{
  strbuf b ("Drange: [%g, %g]", _low, _hi);
  return b;
}

bool
pub_drange_t::match (scalar_obj_t so)
{
  double d;
  if (!so.to_double (&d)) return false;
  return (d >= _low && d <= _hi);
}

void
pub_range_t::eval_obj (pbuf_t *ps, penv_t *e, u_int d) const
{
  ps->add (to_str ());
}

str
pub_urange_t::to_str () const 
{
  strbuf b ("Urange: [");
  b << _low << ", " << _hi << "]";
  return b;
}

bool
pub_urange_t::match (scalar_obj_t so)
{
  u_int64_t u;
  if (!so.to_uint64 (&u)) return false;
  return (u >= _low && u <= _hi);
}


