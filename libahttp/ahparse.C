// -*-c++-*-
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

#include "ahparse.h"

http_parser_base_t::~http_parser_base_t ()
{
  if (tocb) {
    timecb_remove (tocb);
    tocb = NULL;
  }
  *destroyed = true;
}

void
http_parser_base_t::parse (cbi c)
{
  cb = c;
  tocb = delaycb (timeout, 0, wrap (this, &http_parser_base_t::clnt_timeout));
  hdr_p ()->parse (wrap (this, &http_parser_base_t::parse_cb1));
}

void
http_parser_base_t::parse_cb1 (int status)
{
  if (status != HTTP_OK) {
    finish (status);
    return;
  }
  v_parse_cb1 (status);
}

void
http_parser_base_t::finish (int status)
{
  if (tocb) {
    timecb_remove (tocb);
    tocb = NULL;
  }
  // If we don't stop the abuf, we might be fooled into parsing
  // again on an EOF.
  stop_abuf ();

  // XXX -- we (stupidly) have a cyclic data structure here; that is,
  // we might have a refcounted this wrapped in the callback given.
  // thus, if we haven't be destroyed, we'll need to set the CB equal 
  // to NULL.  in some cases, so doing will cause the object to be 
  // destroyed.
  //
  // this seems to be the safest way to go about this. we'll have cleared
  // cb before we make the (potentially destructive) call to tcbi.
  // when tcbi goes out of scope as the function returns, this might
  // cause this object to be deleted. 

  cbi tcb = cb;
  cb = NULL;
  (*tcb) (status);

  // for those who add more code to this function ....bad idea; 
  // the call to (*tcb) might have very well deleted us; it's crucial
  // not to touch class variables after the (*tcb) call.
}

void
http_parser_base_t::stop_abuf ()
{
  abuf.finish ();
}

void
http_parser_full_t::finish2 (int s1, int s2)
{
  if (s1 != HTTP_OK)
    finish (s1);
  else if (s2 != HTTP_OK)
    finish (s2);
  else
    finish (HTTP_OK);
}

cbi::ptr
http_parser_full_t::prepare_post_parse (int status)
{
  if (hdr.contlen > int (ok_reqsize_limit)) {
    finish (HTTP_NOT_ALLOWED);
    return NULL;
  } else if (hdr.contlen < 0)
    hdr.contlen = ok_reqsize_limit -1;
  
  abuf.setlim (hdr.contlen);
  return wrap (this, &http_parser_full_t::finish2, status);
}

void
http_parser_cgi_t::v_parse_cb1 (int status)
{
  if (hdr.mthd == HTTP_MTHD_POST) {
    
    cgi_t *post_cgi;

    if (_union_mode) {

      cgi = &_union_cgi;
      post_cgi = &_union_cgi;

      // need to reset interior state for new parsing.
      _union_cgi.reset_state ();

    } else {
      cgi = &post;
      post_cgi = &post;
    }
    
    cbi::ptr pcb;
    if (!(pcb = prepare_post_parse (status)))
      return;

    str boundary;
    if (cgi_mpfd_t::match (hdr, &boundary)) {
      if (mpfd_flag) {
	mpfd = cgi_mpfd_t::alloc (&abuf, hdr.contlen, boundary);
	cgi = mpfd;
	mpfd->parse (pcb);
      } else {
	warn << "file upload attempted when not permitted\n";
	finish (HTTP_NOT_ALLOWED);
      }
    } else 
      post_cgi->parse (pcb);
  } else if (hdr.mthd == HTTP_MTHD_GET) {
    if (_union_mode) {
      cgi = &_union_cgi;
    } else {
      cgi = &url;
    }
    finish (status);
  } else {
    finish (HTTP_NOT_ALLOWED);
  }
}

void
http_parser_base_t::clnt_timeout ()
{
  tocb = NULL;
  v_cancel ();
  finish (HTTP_TIMEOUT);
}


#ifdef HAVE_EXPAT

void
http_parser_xml_t::v_cancel ()
{
  hdr.cancel ();
  _xml.cancel ();
}

void
http_parser_xml_t::v_parse_cb1 (int status)
{
  if (hdr.mthd == HTTP_MTHD_POST) {

    cbi::ptr pcb;
    if (!(pcb = prepare_post_parse (status)))
      return;
    
    _xml.init ();
    _xml.parse (pcb);

  } else {
    finish (HTTP_NOT_ACCEPTABLE);
  }
}

void
http_parser_cgi_t::set_union_mode (bool b)
{
  _union_mode = b;
  hdr.set_url (_union_mode ? &_union_cgi : &url);
}

#endif /* HAVE_EXPAT */

