
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

//-----------------------------------------------------------------------

class mybuf_t {
public:
  mybuf_t () {}
  str to_str () { return _b; }
  void add_s (str s) { _hold.push_back (s); _b << s; }
  void add_ch (char ch) { _b.buf (&ch, 1); }
private:
  strbuf _b;
  vec<str> _hold;
};

//-----------------------------------------------------------------------

static bool
filter_tags_match (const char *start, const char *end, const bhash<str> &list)
{
  while (start < end && isspace (*start)) start ++;
  while (start < end && isspace (*end)) end--;
  return list[str (start, end - start + 1)];
}

//-----------------------------------------------------------------------

static bool
find_space_in (const char *start, const char *end)
{
  while (start <= end) 
    if (isspace (*start))
      return true;
  return false;
}

//-----------------------------------------------------------------------

static bhash<str> safe_entity_list;

//-----------------------------------------------------------------------

static void
init_safe_entity_list ()
{
  static const char *ents[] = { "lt", "gt", "amp", "#35", "quot" , NULL };
  for (const char **ep = ents; *ep; ep++) {
    safe_entity_list.insert (*ep);
  }
}

//-----------------------------------------------------------------------

static bool
is_safe_entity (const char *start, const char *end)
{
  str s (start, end - start + 1);
  if (!safe_entity_list.size ()) {
    init_safe_entity_list ();
  }
  return safe_entity_list[s];
}

//-----------------------------------------------------------------------

str
filter_tags (const str &in, const bhash<str> &exceptions)
{
  const char *cp = in.cstr ();
  const char *ep = in.cstr () + in.len ();
  if (*ep != '\0') { return NULL; }

  mybuf_t buf;

  while (*cp) {
    
    str add;
    
    switch (*cp) {
    case '<':
      {
	
	const char *inner = cp + 1;
	const char *etp = strchr (inner, '>');
	
	if (!etp) { 
	  cp = etp;
	} else {
	  const char *outter = etp - 1;
	  if (inner[1] == '/') { inner++; }
	  if (*outter == '/') { outter++; }
	  if (filter_tags_match (inner, outter, exceptions)) {
	    buf.add_s (str (cp, etp - cp + 1));
	  }
	  cp = etp + 1;
	}
      }
      break;
      
    case '&':
      {
	
	const char *inner = cp + 1;
	const char *etp = strchr (inner, ';');
	if (!etp || find_space_in (inner, etp)) {
	  buf.add_ch (*cp++);
	} else {
	  if (is_safe_entity (inner, etp)) {
	    buf.add_s (str (cp, etp - cp + 1));
	  }
	  cp = etp + 1;
	}
      }
      break;
      
    case '#':
      buf.add_s ("&#35;");
      cp ++;
      break;
      
    case '"':
      buf.add_s ("&quot;");
      cp ++;
      
    default:
      buf.add_ch (*cp++);
      break;
      
    }
  }
  return buf.to_str ();
}


//-----------------------------------------------------------------------
