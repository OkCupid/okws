
#include "pescape.h"
#include "qhash.h"

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

//=========================================================================
//
//  Various routines for filtering HTML while leaving in some tags.
//

//-----------------------------------------------------------------------

bool
html_filter_bhash_t::match (const char *start, const char *end) const
{
  while (start < end && isspace (*start)) start ++;
  while (start < end && isspace (*end)) end--;
  return (*_tab)[str (start, end - start + 1)];
}

//-----------------------------------------------------------------------

bool
html_filter_t::find_space_in (const char *start, const char *end)
{
  while (start < end) 
    if (isspace (*start++))
      return true;
  return false;
}

//-----------------------------------------------------------------------

const bhash<str> &
html_filter_t::safe_entity_list ()
{
  static bhash<str> list;
  if (!list.size ()) {
    static const char *ents[] = { "lt", "gt", "amp", "#35", "quot" , NULL };
    for (const char **ep = ents; *ep; ep++) {
      list.insert (*ep);
    }
  }
  return list;
}

//-----------------------------------------------------------------------

bool
html_filter_t::is_safe_entity (const char *start, const char *end)
{
  str s (start, end - start);
  return safe_entity_list ()[s];
}

//-----------------------------------------------------------------------

void
html_filter_bhash_t::handle_tag (buf_t *out, const char **cpp, const char *ep)
{
  const char *cp = *cpp;
  
  const char *inner = cp + 1;
  const char *etp = strchr (inner, '>');
	
  if (!etp) { 
    cp = ep;
  } else {
    const char *outter = etp - 1;
    if (inner[1] == '/') { inner++; }
    if (*outter == '/') { outter++; }
    if (match (inner, outter)) {
      out->add_s (str (cp, etp - cp + 1));
      cp = etp + 1;
    } else {
      out->add_cc ("&lt;");
      cp ++;
    }
  }
  *cpp = cp;
}

//-----------------------------------------------------------------------

void
html_filter_rxx_t::handle_tag (buf_t *out, const char **cpp, const char *ep)
{
  const char *cp = *cpp;
  bool go;
#if SFSLITE_AT_VERSION(1,2,8,5)
  go = _rxx->search_cstr (cp, ep - cp, PCRE_ANCHORED);
#else
  go = _rxx->search (cp, PCRE_ANCHORED);
#endif
  if (go) {
    size_t len = _rxx->end (0);
    out->add_cc (cp, len, true);
    cp += len;
  } else {
    out->add_cc ("&lt;");
    cp ++;
  }
  *cpp = cp;
}

//-----------------------------------------------------------------------

void 
filter_buf_t::add_cc (const char *p, ssize_t len, bool cp) {
  if (len < 0) { len = strlen (p); }
  if (cp) { _b.tosuio ()->copy (p, len); }
  else { _b.buf (p, len); }
}

//-----------------------------------------------------------------------

void 
filter_buf_t::add_ch (char c)
{
  _b.tosuio ()->copy (&c, 1);
}

//-----------------------------------------------------------------------

str
html_filter_t::run (const str &in)
{
  if (!in) return in;

  const char *cp = in.cstr ();
  const char *ep = in.cstr () + in.len ();
  if (*ep != '\0') { return NULL; }

  buf_t buf;

  while (*cp) {
    
    str add;
    
    switch (*cp) {
    case '<':
      handle_tag (&buf, &cp, ep);
      break;
	
    case '&':
      {
	const char *inner = cp + 1;
	const char *etp = strchr (inner, ';');
	if (etp && is_safe_entity (inner, etp)) {
	  buf.add_s (str (cp, etp - cp + 1));
	  cp = etp + 1;
	} else {
	  buf.add_cc ("&amp;");
	  cp ++;
	}
      }
      break;
      
    case '#':
      buf.add_cc ("&#35;");
      cp ++;
      break;
      
    case '"':
      buf.add_cc ("&quot;");
      cp ++;
      
    default:
      buf.add_ch (*cp++);
      break;
      
    }
  }
  return buf.to_str ();
}

//
//=======================================================================

str
htmlspecialchars (const str &in)
{
  if (!in) return in;

  filter_buf_t buf;
  const char *bp = in.cstr ();
  const char *ep = bp + in.len ();

  for ( ; bp < ep; bp++) {
    switch (*bp) {
    case '&': buf.add_cc ("&amp;"); break;
    case '<': buf.add_cc ("&lt;"); break;
    case '>': buf.add_cc ("&gt;"); break;
    case '\"': buf.add_cc ("&quot;"); break;
    case '\'': buf.add_cc ("&#039;"); break;
    default: buf.add_ch (*bp); break;
    }
  }
  return buf.to_str ();
}

//-----------------------------------------------------------------------

