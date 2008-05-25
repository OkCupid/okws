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

#ifndef _LIBAHTTP_OKXMLPARSE_H
#define _LIBAHTTP_OKXMLPARSE_H


// Only use this library if we have EXPAT support
#include "okwsconf.h"
# ifdef HAVE_EXPAT

#include "async.h"
#include "ihash.h"
#include "ctype.h"
#include "aparse.h"
#include "expat.h"
#include "okxmldata.h"

// see additional codes in <expat.h>
enum { XML_PARSE_OK = 0,
       XML_PARSE_BAD_NESTING = 101,
       XML_PARSE_UNKNOWN_ELEMENT = 102,
       XML_PARSE_UNEXPECTED_CHARDATA = 103,
       XML_PARSE_UNBALANCED = 104,
       XML_PARSE_UNMATCHED = 105,
       XML_PARSE_CLOSE_ERROR = 106,
       XML_PARSE_BAD_CHARDATA = 107,
       XML_PARSE_UNEXPECTED_CDATA = 108,
       XML_PARSE_BAD_CDATA = 109 };

class xml_req_parser_t : public async_parser_t {
public:

  xml_req_parser_t (abuf_src_t *s) 
    : async_parser_t (s), 
      _xml_parser_init (false),
      _status (XML_PARSE_OK) {}

  xml_req_parser_t (abuf_t *a) 
    : async_parser_t (a), 
      _xml_parser_init (false),
      _status (XML_PARSE_OK) {}

  void init (const char *encoding = NULL);

  void start_element (const char *name, const char **atts);
  void end_element (const char *name);
  void found_data (const char *buf, int len);
  void start_cdata ();
  void end_cdata ();

  virtual ptr<xml_element_t> generate (const char *nm, const char **atts);
  virtual void v_init ();

  ptr<xml_top_level_t> top_level () { return _top_level; }
  ptr<const xml_top_level_t> top_level () const { return _top_level; }
  str errmsg () const;
  int errcode () const { return _status; }

  void cancel ();

  virtual ~xml_req_parser_t ();

protected:
  xml_element_t *active_el () { return _stack.back (); }
  void push_el (ptr<xml_element_t> e) { _stack.push_back (e); }
  ptr<xml_element_t> pop_el () { return _stack.pop_back (); }

  void parse_error (int s, str m);

private:
  vec<ptr<xml_element_t> > _stack;

  virtual void parse_guts ();
  bool _xml_parser_init;
  XML_Parser _xml_parser;
  ptr<xml_top_level_t> _top_level;

  int                _status;
  str                _err_msg;
};


# endif /* HAVE_EXPAT */
#endif  /* LIBAHTTP_OKXMLPARSE_H */
