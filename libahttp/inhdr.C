
#include "inhdr.h"
#include "httpconst.h"
#include <ctype.h>
#include "rxx.h"

methodmap_t methodmap;

abuf_stat_t
http_inhdr_t::delimit_word (str *wrd, bool qms)
{
  int ch;
  abuf_stat_t ret = ABUF_OK;
  bool flag = true;
  for ( ; pcp < endp && flag; flag && pcp++) {
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
http_inhdr_t::parse (cbi::ptr c)
{
  assert (!parsing);
  abuf->init (wrap (this, &http_inhdr_t::can_read_cb));
  parsing = true;
  pcb = c;
  if (dataready)
    _parse ();
}

void
http_inhdr_t::cancel ()
{
  abuf->cancel ();
  pcb = NULL;
}

void
http_inhdr_t::can_read_cb ()
{
  abuf->can_read ();
  if (parsing)
    _parse ();
  else
    dataready = true;
}

#define INC_STATE \
   state = static_cast<hdrst_t> (state + 1);

void
http_inhdr_t::ext_parse_cb ()
{
  INC_STATE;
  abuf->set_ignore_finish (false);
  parse (pcb);
}

void
http_inhdr_t::_parse ()
{
  abuf_stat_t r = ABUF_OK;
  while (r == ABUF_OK && !hdrend) {
    switch (state) {
    case HDRST_START:
      abuf->mirror (scr2, SCR2_LEN);
      r = delimit_word (&tmthd);
      break;
    case HDRST_SPC1:
      r = abuf->skip_hws (1);
      break;
    case HDRST_TARGET:
      r = delimit_word (&target, uri ? true : false);
      if (!uri && r == ABUF_OK)
	INC_STATE; // skip past HDRST_URIDAT
      break;
    case HDRST_URIDAT:
      if (abuf->expectchar ('?') == ABUF_OK) {
	abuf->set_ignore_finish (true);
	parsing = false;
	uri->set_uri_mode (true);
	uri->parse (wrap (this, &http_inhdr_t::ext_parse_cb));
	return;
      }
      break;
    case HDRST_SPC2:
      r = abuf->skip_hws (1);
      break;
    case HDRST_OPTPARAM:
      if (!eol ()) r = delimit_word (&vers);
      if (vers) { // XXX will only work for versions 1.0 and 1.1
	char c = vers[vers.len () - 1];
	if (c >= '0' && c <= '9') nvers = c - '0';
      }
      line1 = abuf->end_mirror ();
      break;
    case HDRST_EOL1:
      if (!gobble_eol ())
	r = ABUF_PARSE_ERR;
      break;
    case HDRST_KEY:
      if (gobble_eol ()) {
	hdrend = true;
	break;
      }
      r = delimit_key (&key);
      break;
    case HDRST_SPC3:
      r = abuf->skip_hws (1);
      break;
    case HDRST_VALUE:
      if (cookie && iscookie ()) {
	parsing = false;
	abuf->set_ignore_finish (true);
	noins = true;
	cookie->parse (wrap (this, &http_inhdr_t::ext_parse_cb));
	return;
      }
      r = delimit_val (&val);
      break;
    case HDRST_EOL2:
      if (noins)
	noins = false;
      else if (key)
	insert (key, val);
      else
	r = ABUF_PARSE_ERR;
      key = val = NULL;
      if (!gobble_eol ())
	r = ABUF_PARSE_ERR;
      state = HDRST_EOL1; // = HDRST_KEY - 1
      break;
    default:
      r = ABUF_PARSE_ERR;
    }
    if (r == ABUF_OK)
      INC_STATE;
  }

  // do sanity checks here?
  int status;
  switch (r) {
  case ABUF_EOF:
    if (state == HDRST_KEY) {
      status = HTTP_OK;
      break;
    }
  case ABUF_PARSE_ERR:
    status = HTTP_BAD_REQUEST;
    break;
  case ABUF_WAIT:
    return; // wait for a Callback (we hope)
  default:
    status = HTTP_OK;
  }
  fixup ();
  if (pcb) 
    (*pcb) (status);
}

void
http_inhdr_t::fixup () 
{
  mthd = methodmap.lookup (tmthd);
  if (!lookup ("content-length", &reqsize))
    reqsize = -1;
}

bool
http_inhdr_t::iscookie () const
{
  return (key && key.len () == 6 && mystrlcmp (key, "cookie"));
}

bool
http_inhdr_t::eol ()
{
  int ch = abuf->peek ();
  return (ch == '\r' || ch == '\n');
}

bool
http_inhdr_t::gobble_eol ()
{
  int ch = abuf->get ();
  if (ch == '\n') return true;
  else if (ch == '\r') return (abuf->get () == '\n');

  abuf->unget (); // XXX doesn't unget the '\r' in certain cases.. oh well!
  return false;
}

abuf_stat_t
http_inhdr_t::delimit_val (str *v)
{
  abuf_stat_t ret = ABUF_OK;
  int ch;
  bool flag = true;
  for ( ; pcp < endp && flag; flag && pcp++) {
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
http_inhdr_t::delimit_key (str *k)
{
  abuf_stat_t ret = ABUF_OK;
  int ch;
  bool flag = true;
  for ( ; pcp < endp && flag; flag && pcp++) {
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

methodmap_t::methodmap_t ()
{
  map.insert ("GET", HTTP_MTHD_GET);
  map.insert ("POST", HTTP_MTHD_POST);
  map.insert ("PUT", HTTP_MTHD_PUT);
  map.insert ("DELETE", HTTP_MTHD_DELETE);
}

http_method_t
methodmap_t::lookup (const str &s) const
{
  const http_method_t *v = map[s];
  if (!v)
    return HTTP_MTHD_NONE;
  return *v;
}

static rxx gzip_rxx ("gzip(,|\\s*$)");

bool
http_inhdr_t::takes_gzip () const
{
  str s;
  return (get_vers () > 0 && lookup ("accept-encoding", &s) 
	  && gzip_rxx.search (s));
}
