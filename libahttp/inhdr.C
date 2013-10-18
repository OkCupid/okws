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
#include "parseopt.h"

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
      abuf->mirror (_scr2->buf (), _scr2->len ());
      r = delimit_word (&tmthd);
      break;
    case INHDRST_SPC1:
      r = abuf->skip_hws (1);
      break;
    case INHDRST_TARGET:
      r = delimit_word (&target, _parse_query_string);
      if (!_parse_query_string && r == ABUF_OK) {
	state = INHDRST_SPC2;
	inc = false;
      }
      break;
    case INHDRST_URIDAT:
      r = abuf->expectchar ('?');
      if (r == ABUF_OK) {
	ptr<cgi_t> url = get_url ();
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
      // keepalived doesn't put a space between it's User-Agent
      // and it, so let's accommodate it here....
      r = abuf->skip_hws (0);
      break;
    case INHDRST_VALUE:
      if (ok_http_parse_cookies && get_cookie () && iscookie ()) {
	noins = true;
	get_cookie ()->parse (wrap (this, &http_inhdr_t::ext_parse_cb));
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

  if (status == HTTP_BAD_REQUEST && 
      r == ABUF_EOF && 
      clean_pipeline_eof_state ()) {

    status = HTTP_PIPELINE_EOF;
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
  
  str tmp;

  _conn_mode = 
    ((tmp = get_connection ()) && cicmp (tmp, "keep-alive")) ? 
    HTTP_CONN_KEEPALIVE : HTTP_CONN_CLOSED;
}

methodmap_t::methodmap_t ()
{
  map.insert ("GET", HTTP_MTHD_GET);
  map.insert ("POST", HTTP_MTHD_POST);
  map.insert ("PUT", HTTP_MTHD_PUT);
  map.insert ("DELETE", HTTP_MTHD_DELETE);
  map.insert ("HEAD", HTTP_MTHD_HEAD);
  map.insert ("OPTIONS", HTTP_MTHD_OPTIONS);
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
static rxx safari_rxx ("\\bSafari/(\\d+)(\\.[0-9.]+)?\\b");
static rxx chrome_rxx ("\\bChrome\\b");

//-----------------------------------------------------------------------

bool
http_inhdr_t::takes_gzip () const
{
  str s, ua;
  ua = get_user_agent ();
  return (get_vers () > 0 
	  && lookup ("accept-encoding", &s) 
	  && gzip_rxx.search (s) 
	  && ua
	  && !netscape4_rxx.search (ua));
}

//-----------------------------------------------------------------------

bool
http_inhdr_t::has_broken_chunking () const
{
  str ua = get_user_agent ();
  int v;
  bool ret = false;

  // MK 2011/4/22 -- Jase's version of Safari, some unknown version
  // of Safari that predates May 2009, couldn't handle chunking support.
  // So for versions of Safari less than 525, disable it.  Make sure it's
  // not Chrome though, since Chrome pretends to be Safari.
  if (ua && 
      !ok_gzip_chunking_old_safaris &&
      !chrome_rxx.search (ua) && 
      safari_rxx.search (ua) &&
      convertint (safari_rxx[1], &v) && 
      v <= 525) {

    ret = true;
  }

  return ret;
}

//-----------------------------------------------------------------------

http_conn_mode_t 
http_inhdr_t::get_conn_mode () const
{ return _conn_mode; }

//-----------------------------------------------------------------------

void
http_inhdr_t::set_reqno (u_int i, bool pipelining, htpv_t prev_vers)
{
  _reqno = i;
  _pipeline_eof_ok = (i > 1 && pipelining && prev_vers == 1);
}

//-----------------------------------------------------------------------

str
http_inhdr_t::get_connection () const
{
  str tmp;
  lookup ("connection", &tmp);
  return tmp;
}

//-----------------------------------------------------------------------

void
http_inhdr_t::v_debug ()
{}

//-----------------------------------------------------------------------

bool
http_inhdr_t::clean_pipeline_eof_state () const
{
  return (state == INHDRST_START && _pipeline_eof_ok);
}

//-----------------------------------------------------------------------

int
http_inhdr_t::timeout_status () const
{
  return (clean_pipeline_eof_state () ? HTTP_PIPELINE_CLEAN_TIMEOUT :
	  HTTP_TIMEOUT);
}

//-----------------------------------------------------------------------

ptr<ok::scratch_handle_t> 
http_inhdr_t::alloc_scratch2 ()
{
  if (!_scratch2) {
    _scratch2 = ok::alloc_scratch (ok_http_inhdr_buflen_sml);
  }
  return _scratch2;
}

//-----------------------------------------------------------------------

ptr<cgi_t>
http_inhdr_t::get_cookie ()
{
  if (!_cookie) {
    _cookie = cgi_t::alloc (get_abuf (), true, alloc_scratch2 ());
  }
  return _cookie;
}

//-----------------------------------------------------------------------

ptr<const cgi_t>
http_inhdr_t::get_cookie () const
{
  ptr<const cgi_t> out;
  out = _cookie;
  if (!out) out = cgi_t::global_empty ();
  return out;
}

//-----------------------------------------------------------------------


ptr<cgi_t>
http_inhdr_t::get_url ()
{
  if (!_url) {
    _url = cgi_t::alloc (get_abuf (), false, alloc_scratch2 ());
  }
  return _url;
}

//-----------------------------------------------------------------------

str
http_inhdr_t::get_referrer (bool null_ok) const
{
  str ret;
  if (!lookup ("referer", &ret) && !null_ok)
    ret = "";
  return ret;
}

//-----------------------------------------------------------------------

str
http_inhdr_t::get_user_agent (bool null_ok) const
{
  str ret;
  if (!lookup ("user-agent", &ret) && !null_ok)
    ret = "";
  return ret;
}

//-----------------------------------------------------------------------
