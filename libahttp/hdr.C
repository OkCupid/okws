
#include "hdr.h"
#include "httpconst.h"
#include <ctype.h>

abuf_stat_t
http_hdr_t::delimit_word (str *wrd, bool qms)
{
  int ch;
  abuf_stat_t ret = ABUF_OK;
  bool flag = true;
  for ( ; pcp < endp && flag; pcp += (flag ? 1 : 0)) {
    ch = abuf->get ();
    switch (ch) {
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      flag = false;
      break;
    case '?':
      if (qms) {
	ret = ABUF_OK;
	flag = false;
      } else {
	*pcp = ch;
      }
      break;
    case ABUF_EOFCHAR:
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      ret = ABUF_OK;
      flag = false;
      break;
    default:
      *pcp = ch;
      break;
    }
  }
  if (pcp == endp)
    ret = ABUF_OVERFLOW;
  if (ret == ABUF_OK) {
    *wrd = str (scratch, pcp - scratch);
    pcp = scratch;
    abuf->unget ();
  }
  return ret;
}

void
http_hdr_t::parse (cbi::ptr c)
{
  assert (!parsing);
  abuf->init (wrap (this, &http_hdr_t::can_read_cb));
  parsing = true;
  pcb = c;
  if (dataready)
    _parse ();
}

void
http_hdr_t::cancel ()
{
  abuf->cancel ();
  pcb = NULL;
}

void
http_hdr_t::can_read_cb ()
{
  abuf->can_read ();
  if (parsing)
    _parse ();
  else
    dataready = true;
}

void
http_hdr_t::fixup () 
{
  if (!lookup ("content-length", &contlen))
    contlen = -1;
}

bool
http_hdr_t::eol ()
{
  int ch = abuf->peek ();
  return (ch == '\r' || ch == '\n');
}

bool
http_hdr_t::gobble_eol ()
{
  int ch = abuf->get ();
  if (ch == '\n') return true;
  else if (ch == '\r') return (abuf->get () == '\n');

  abuf->unget (); // XXX doesn't unget the '\r' in certain cases.. oh well!
  return false;
}

abuf_stat_t
http_hdr_t::delimit_val (str *v)
{
  abuf_stat_t ret = ABUF_OK;
  int ch;
  bool flag = true;
  for ( ; pcp < endp && flag; pcp += (flag ? 1 : 0)) {
    ch = abuf->get ();
    switch (ch) {
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      flag = false;
      break;
    case ABUF_EOFCHAR:
      ret = ABUF_EOF;
      flag = false;
      break;
    case '\r':
    case '\n':
      abuf->unget ();
      flag = false;
      break;
    default:
      *pcp = ch;
      break;
    }
  }
  if (pcp == endp)
    ret = ABUF_OVERFLOW;
  if (ret == ABUF_OK) {
    *v = str (scratch, pcp - scratch);
    pcp = scratch;
  }
  return ret;
}

abuf_stat_t
http_hdr_t::delimit_key (str *k)
{
  abuf_stat_t ret = ABUF_OK;
  int ch;
  bool flag = true;
  for ( ; pcp < endp && flag; pcp += (flag ? 1 : 0)) {
    ch = abuf->get ();
    switch (ch) {
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      flag = false;
      break;
    case ':':
      ret = ABUF_OK;
      flag = false;
      break;
    case ABUF_EOFCHAR:
      ret = ABUF_EOF;
      flag = false;
      break;
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      ret = ABUF_PARSE_ERR;
      flag = false;
      break;
    default:
      *pcp = tolower (ch);
      break;
    }
  }
  if (pcp == endp)
    ret = ABUF_OVERFLOW;
  if (ret == ABUF_OK) {
    *k = str (scratch, pcp - scratch);
    pcp = scratch;
  }
  return ret;
}
