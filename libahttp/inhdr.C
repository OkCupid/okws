/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "inhdr.h"
#include "httpconst.h"
#include <ctype.h>
#include "rxx.h"

methodmap_t methodmap;

#define INC_STATE \
   state = static_cast<inhdrst_t> (state + 1);

void
http_inhdr_t::ext_parse_cb (int status)
{
  INC_STATE;
  resume ();
}

void
http_inhdr_t::parse_guts ()
{
  abuf_stat_t r = ABUF_OK;
  bool inc;
  int status = HTTP_BAD_REQUEST;

  while (r == ABUF_OK) {
    inc = true;
    switch (state) {
    case INHDRST_START:
      abuf->mirror (scr2, SCR2_LEN);
      r = delimit_word (&tmthd);
      break;
    case INHDRST_SPC1:
      r = abuf->skip_hws (1);
      break;
    case INHDRST_TARGET:
      r = delimit_word (&target, url ? true : false);
      if (!url && r == ABUF_OK) {
	state = INHDRST_SPC2;
	inc = false;
      }
      break;
    case INHDRST_URIDAT:
      r = abuf->expectchar ('?');
      if (r == ABUF_OK) {
	url->set_uri_mode (true);
	url->parse (wrap (this, &http_inhdr_t::ext_parse_cb));
	return;
      } else if (r == ABUF_NOMATCH)
	r = ABUF_OK;
      break;
    case INHDRST_SPC2:
      // Maybe there's an end-of-the-line here...
      if ((r = eol ()) != ABUF_OK) 
	r = abuf->skip_hws (1);
      break;
    case INHDRST_OPTPARAM:
      r = eol ();
      if (r == ABUF_NOMATCH) { // No EOL found
	r = delimit_word (&vers);
	// XXX will only work for versions 1.0 and 1.1
	if (vers && vers.len () > 0) { 
	  char c = vers[vers.len () - 1];
	  if (c >= '0' && c <= '9') nvers = c - '0';
	}
      } 
      break;
    case INHDRST_EOL1:
      line1 = abuf->end_mirror ();
      r = require_crlf ();
      break;
    case INHDRST_KEY:
      r = gobble_crlf ();
      if (r == ABUF_OK) {
	status = HTTP_OK;
	r = ABUF_EOF;
	break;
      } else if (r == ABUF_NOMATCH) // found non-EOL
	r = delimit_key (&key);
      // else we're waiting or found EOF or found stray '\r' in hdr
      break;
    case INHDRST_SPC3:
      r = abuf->skip_hws (1);
      break;
    case INHDRST_VALUE:
      if (cookie && iscookie ()) {
	noins = true;
	cookie->parse (wrap (this, &http_inhdr_t::ext_parse_cb));
	return;
      }
      r = delimit_val (&val);
      break;
    case INHDRST_EOL2A:
      if (noins)
	noins = false;
      else if (key)
	insert (key, val);
      else
	r = ABUF_PARSE_ERR;
      key = val = NULL;
      break;
    case INHDRST_EOL2B:
      r = require_crlf ();
      if (r == ABUF_OK) {
	state = INHDRST_KEY;
	inc = false;
      }
      break;
    default:
      r = ABUF_PARSE_ERR;
      break;
    }
    if (r == ABUF_OK && inc)
      INC_STATE;
  }

  // on ABUF_WAIT, we still need more data.  on any other value for
  // r (such as ABUF_EOF, ABUF_PARSE_ERR, ABUF_NOMATCH), we need to stop
  // HTTP header parsing.
  switch (r) {
  case ABUF_WAIT:
    return;
  case ABUF_OVERFLOW:
    status = HTTP_REQ_TOO_BIG;
    break;
  case ABUF_EOF:
    if (abuf->overflow ())
      status = HTTP_REQ_TOO_BIG;
  default:
    break;
  }

  if (status == HTTP_OK)
    fixup ();
  finish_parse (status);
}

void
http_inhdr_t::fixup () 
{
  if (!lookup ("content-length", &contlen))
    contlen = -1;
  mthd = methodmap.lookup (tmthd);
}

methodmap_t::methodmap_t ()
{
  map.insert ("GET", HTTP_MTHD_GET);
  map.insert ("POST", HTTP_MTHD_POST);
  map.insert ("PUT", HTTP_MTHD_PUT);
  map.insert ("DELETE", HTTP_MTHD_DELETE);
  map.insert ("HEAD", HTTP_MTHD_HEAD);
}

post_methodmap_t::post_methodmap_t ()
{
  map.insert (ok_http_urlencoded, POST_MTHD_URLENCODED);
  map.insert (ok_http_multipart, POST_MTHD_MULTIPART_FORM);
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
static rxx netscape4_rxx ("Mozilla/4.0[678]");

bool
http_inhdr_t::takes_gzip () const
{
  str s, ua;
  return (get_vers () > 0 && lookup ("accept-encoding", &s) 
	  && gzip_rxx.search (s) && lookup ("user-agent", &ua) 
	  && !netscape4_rxx.search (ua));
}

