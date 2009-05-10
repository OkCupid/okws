
#include "pescape.h"

//-----------------------------------------------------------------------

str
json_escape (const str &s, bool addq)
{
  if (!s) return s;

  const char *p1 = s.cstr ();
  const char *p2 = NULL;
  char *buf = New char[2 * s.len () + 3];
  char *dp = buf;
  size_t span;

  if (addq)
    *dp++ = '"';

  char c;
  while ((p2 = strpbrk (p1, "\\\"\n\t\r"))) {
    span = p2 - p1;
    strncpy (dp, p1, span);
    dp += span;
    *dp++ = '\\';

    if (*p2 == '\n') { c = 'n'; } 
    else if (*p2 == '\t') { c = 't'; } 
    else if (*p2 == '\r') { c = 'r'; }
    else { c = *p2; }

    *dp++ = c;
    p2++;
    p1 = p2;
  }
  int len = strlen (p1);
  memcpy (dp, p1, len);
  dp += len;
  if (addq) 
    *dp++ = '"';
  *dp = 0;
  str r (buf);
  
  delete [] buf;
  return r;
}

//-----------------------------------------------------------------------

static char *xss_buf;
static size_t xss_buflen = 0x1000;
static size_t xss_max_buflen = 0x1000000;

//-----------------------------------------------------------------------

str xss_escape (const str &s) { return xss_escape (s.cstr (), s.len ()); }

//-----------------------------------------------------------------------

str
xss_escape (const char *in, size_t inlen)
{
  size_t maxseqlen = 5;

  size_t biggest = maxseqlen * inlen;
  if (xss_buflen < biggest && xss_buflen != xss_max_buflen && xss_buf) {
    delete [] xss_buf;
    xss_buf = NULL;
  }
  if (!xss_buf) {
    while (xss_buflen < biggest && xss_buflen < xss_max_buflen)
      xss_buflen = (xss_buflen << 1);
    xss_buflen = min<size_t> (xss_buflen, xss_max_buflen);
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
    case '&':
      inc = sprintf (op, "&#38;");
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

//-----------------------------------------------------------------------
