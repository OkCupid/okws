
#include "inhdr.h"
#include "httpconst.h"
#include <ctype.h>
#include "rxx.h"

methodmap_t methodmap;

#define INC_STATE \
   state = static_cast<inhdrst_t> (state + 1);

void
http_inhdr_t::ext_parse_cb ()
{
  INC_STATE;
  parse (pcb);
}

void
http_inhdr_t::_parse ()
{
  abuf_stat_t r = ABUF_OK;
  while (r == ABUF_OK && !hdrend) {
    switch (state) {
    case INHDRST_START:
      abuf->mirror (scr2, SCR2_LEN);
      r = delimit_word (&tmthd);
      break;
    case INHDRST_SPC1:
      r = abuf->skip_hws (1);
      break;
    case INHDRST_TARGET:
      r = delimit_word (&target, uri ? true : false);
      if (!uri && r == ABUF_OK)
	INC_STATE; // skip past INHDRST_URIDAT
      break;
    case INHDRST_URIDAT:
      if (abuf->expectchar ('?') == ABUF_OK) {
	parsing = false;
	uri->set_uri_mode (true);
	uri->parse (wrap (this, &http_inhdr_t::ext_parse_cb));
	return;
      }
      break;
    case INHDRST_SPC2:
      r = abuf->skip_hws (1);
      break;
    case INHDRST_OPTPARAM:
      if (!eol ()) r = delimit_word (&vers);
      if (vers) { // XXX will only work for versions 1.0 and 1.1
	char c = vers[vers.len () - 1];
	if (c >= '0' && c <= '9') nvers = c - '0';
      }
      line1 = abuf->end_mirror ();
      break;
    case INHDRST_EOL1:
      if (!gobble_eol ())
	r = ABUF_PARSE_ERR;
      break;
    case INHDRST_KEY:
      if (gobble_eol ()) {
	hdrend = true;
	break;
      }
      r = delimit_key (&key);
      break;
    case INHDRST_SPC3:
      r = abuf->skip_hws (1);
      break;
    case INHDRST_VALUE:
      if (cookie && iscookie ()) {
	parsing = false;
	noins = true;
	cookie->parse (wrap (this, &http_inhdr_t::ext_parse_cb));
	return;
      }
      r = delimit_val (&val);
      break;
    case INHDRST_EOL2:
      if (noins)
	noins = false;
      else if (key)
	insert (key, val);
      else
	r = ABUF_PARSE_ERR;
      key = val = NULL;
      if (!gobble_eol ())
	r = ABUF_PARSE_ERR;
      state = INHDRST_EOL1; // = INHDRST_KEY - 1
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
    if (state == INHDRST_KEY) {
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
  http_hdr_t::fixup ();

  mthd = methodmap.lookup (tmthd);
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

