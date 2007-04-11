// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

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

#ifndef _LIBWEB_OKWCXML_H
#define _LIBWEB_OKWCXML_H


#include "okwc3.h"
#include "okxmlparse.h"
#include "okxmlobj.h"

// Can only compile this library if we have Expat Support
# ifdef HAVE_EXPAT

typedef event<int, xml_inresp_t>::ref okwc3_xml_ev_t;
typedef event<int, str>::ref evis_t;

class okwc3_post_xml_t : public okwc3_post_t {
public:
  okwc3_post_xml_t () : okwc3_post_t () {}
  zbuf &zb () { return _zb; }
  const zbuf &zb () const { return _zb; }
  size_t len () const { return _zb.inflated_len (); }
  void output (strbuf &b) const { _zb.output (&b, false); }
private:
  mutable zbuf _zb;
};

class okwc3_req_xml_t : public okwc3_req_t {
public:
  okwc3_req_xml_t (const str &hn, const str &fn, cgi_t *c = NULL)
    : okwc3_req_t (hn, fn, 1, c) {}
  zbuf &zb () { return _post.zb (); }
  const zbuf &zb () const { return _post.zb (); }
  const okwc3_post_t *get_post () const { return &_post; }
  str get_type () const { return "text/xml"; }
protected:
  okwc3_post_xml_t _post;
};

class okwc3_resp_xml_t : public okwc3_resp_t {
public:
  okwc3_resp_xml_t () : _parser (&_abuf) {}
  void eat_chunk (size_t sz, evi_t ev) { eat_chunk_T (sz, ev); }
  void finished_meal (int status, evi_t ev);
  ptr<const xml_top_level_t> top_level () 
    const { return _parser.top_level (); }
protected:
  xml_req_parser_t _parser;
  void eat_chunk_T (size_t sz, evi_t ev, CLOSURE);
};

class okwc3_xml_t : public okwc3_t {
public:
  okwc3_xml_t (const str &hn, int port, const str &u)
    : okwc3_t (hn, port), _url (u) {}
  void call (xml_outreq_t req, okwc3_xml_ev_t ev) { call_T (req, ev); }
  void call_dump (xml_outreq_t req, evis_t ev) { call_dump_T (req, ev); }
private:
  void call_T (xml_outreq_t req, okwc3_xml_ev_t ev, CLOSURE);
  void call_dump_T (xml_outreq_t req, evis_t ev, CLOSURE);
  void make_req (xml_outreq_t req, ptr<okwc3_resp_t> resp, evi_t ev, CLOSURE);
  str _url;
};

# endif /* HAVE_EXPAT */

#endif /* _LIBWEB_OKWCXML_H */
