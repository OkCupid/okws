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

namespace okwc3 {

typedef event<int, xml_inresp_t>::ref xml_ev_t;
typedef event<int, str>::ref evis_t;

class post_xml_t : public post_t {
public:
  post_xml_t () : post_t () {}
  zbuf &zb () { return _zb; }
  const zbuf &zb () const { return _zb; }
  size_t len () const { return _zb.inflated_len (); }
  void output (strbuf &b) const { _zb.output (&b, false); }
private:
  mutable zbuf _zb;
};

class req_xml_t : public req_t {
public:
  req_xml_t (ptr<reqinfo_t> ri, cgi_t *c = NULL) : req_t (ri, 1, c) {}
  zbuf &zb () { return _post.zb (); }
  const zbuf &zb () const { return _post.zb (); }
  const post_t *get_post () const { return &_post; }
  str get_type () const { return "text/xml"; }
protected:
  post_xml_t _post;
};

class resp_xml_t : public resp_t {
public:
  resp_xml_t () : _parser (&_abuf) {}
  void eat_chunk (size_t sz, evi_t ev) { eat_chunk_T (sz, ev); }
  void finished_meal (int status, evi_t ev);
  ptr<const xml_top_level_t> top_level () 
    const { return _parser.top_level (); }
protected:
  xml_req_parser_t _parser;
  void eat_chunk_T (size_t sz, evi_t ev, CLOSURE);
};

class agent_xml_t : public agent_t {
public:
  agent_xml_t (const str &hn, int port, const str &u, bool proxied = false,
	       bool https = false);

  void call (xml_outreq_t req, xml_ev_t ev) { call_T (req, ev); }
  void call_dump (xml_outreq_t req, evis_t ev) { call_dump_T (req, ev); }
private:
  void call_T (xml_outreq_t req, xml_ev_t ev, CLOSURE);
  void call_dump_T (xml_outreq_t req, evis_t ev, CLOSURE);
  void make_req (xml_outreq_t req, ptr<resp_t> resp, evi_t ev, CLOSURE);
  ptr<reqinfo_t> _reqinfo;
};

};

# endif /* HAVE_EXPAT */

#endif /* _LIBWEB_OKWCXML_H */
