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


#include "okwc2.h"
#include "okxmlparse.h"
#include "okxmlobj.h"

// Can only compile this library if we have Expat Support
# ifdef HAVE_EXPAT

typedef callback<void, int, xml_inresp_t>::ref okwc2_xml_cb_t;
typedef callback<void, int, str>::ref cbis;

class okwc2_post_xml_t : public okwc2_post_t {
public:
  okwc2_post_xml_t () : okwc2_post_t () {}
  zbuf &zb () { return _zb; }
  const zbuf &zb () const { return _zb; }
  size_t len () const { return _zb.inflated_len (); }
  void output (strbuf &b) const { _zb.output (&b, false); }
private:
  mutable zbuf _zb;
};

class okwc2_req_xml_t : public okwc2_req_t {
public:
  okwc2_req_xml_t (const str &hn, const str &fn, cgi_t *c = NULL)
    : okwc2_req_t (hn, fn, 1, c) {}
  zbuf &zb () { return _post.zb (); }
  const zbuf &zb () const { return _post.zb (); }
  const okwc2_post_t *get_post () const { return &_post; }
  str get_type () const { return "text/xml"; }
protected:
  okwc2_post_xml_t _post;
};

class okwc2_resp_xml_t : public okwc2_resp_t {
public:
  okwc2_resp_xml_t () : _parser (&_abuf) {}
  void eat_chunk (ptr<canceller_t> cncl, size_t sz, cbi cb) 
  { eat_chunk_T (cncl, sz, cb); }
  void finished_meal (ptr<canceller_t> cncl, int status, cbi cb);
  ptr<const xml_top_level_t> top_level () 
    const { return _parser.top_level (); }
protected:
  xml_req_parser_t _parser;
  void eat_chunk_T (ptr<canceller_t> cncl, size_t sz, cbi cb, CLOSURE);
};

class okwc2_xml_t : public okwc2_t {
public:
  okwc2_xml_t (const str &hn, int port, const str &u)
    : okwc2_t (hn, port), _url (u) {}
  void call (xml_outreq_t req, okwc2_xml_cb_t cb, int to = 0)
  { call_T (req, cb, to); }
  void call_dump (xml_outreq_t req, cbis cb, int to = 0)
  { call_dump_T (req, cb, to); }
private:
  void call_T (xml_outreq_t req, okwc2_xml_cb_t cb, int to, CLOSURE);
  void call_dump_T (xml_outreq_t req, cbis cb, int to = 0, CLOSURE);
  void make_req (xml_outreq_t req, ptr<okwc2_resp_t> resp, int to, cbi cb,
		 CLOSURE);
  str _url;
};

# endif /* HAVE_EXPAT */

#endif /* _LIBWEB_OKWCXML_H */
