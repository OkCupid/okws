
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

char *xss_buf = NULL;
size_t xss_buflen = 0x1000;

str xss_filter (const str &s) { return xss_filter (s.cstr (), s.len ()); }

str
xss_filter (const char *in, u_int inlen)
{
  int maxseqlen = 5;

  size_t biggest = maxseqlen * inlen;
  if (xss_buflen < biggest && xss_buflen != XSS_MAX_BUF && xss_buf) {
    delete [] xss_buf;
    xss_buf = NULL;
  }
  if (!xss_buf) {
    while (xss_buflen < biggest && xss_buflen < XSS_MAX_BUF)
      xss_buflen = (xss_buflen << 1);
    xss_buflen = min<size_t> (xss_buflen, XSS_MAX_BUF);
    xss_buf = New char[xss_buflen];
  }

  char *op = xss_buf;
  size_t outlen = 0;
  size_t inc;
  const char *end = in + inlen;
  for (const char *cp = in; 
       cp < end && maxseqlen + outlen < xss_buflen; 
       cp++) {
    switch (*cp) {
    case '<':
      inc = sprintf (op, "&lt;");
      break;
    case '>':
      inc = sprintf (op, "&gt;");
      break;
    case '(':
      inc = sprintf (op, "&#40;");
      break;
    case '&':
      inc = sprintf (op, "&#38;");
      break;
    case '#':
      inc = sprintf (op, "&#35;");
      break;
    default:
      *op = *cp;
      inc = 1;
      break;
    }
    outlen += inc;
    op += inc;
  }
  return str (xss_buf, outlen);
}
