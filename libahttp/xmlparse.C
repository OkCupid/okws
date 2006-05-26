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

#ifndef _LIBAHTTP_OKXML_DATA_H
#define _LIBAHTTP_OKXML_DATA_H

#include "okxml.h"
#include <expat.h>

class xml_tagtab_t {
public:
  xml_tagtab_t ();
  ptr<xml_element_t> generate (const char *s);
private:
  qhash<const char *, ptr<xml_element_t> > _tab;
};

xml_tagtab_t::xml_tagtab_t ()
{
  ptr<xml_element_t> e;

#define I(x)                                \
  {                                         \
    e = New refcounted<xml_##x##_t> ();     \
    _tab.insert (e->name (), e);           \
  }

  I(struct);
  I(array);
  I(double);
  I(str);
  I(base64);
  I(params);
  I(param);
  I(value);
  I(method_call);
  I(name);
  I(member);
  I(data);
  I(method_name);
  I(bool);
  I(int);

#undef I

  // 'int' has 2 names...
  e = New refcounted<xml_int_t> ();
  _tab.insert ("i4", e);

}

ptr<xml_element_t>
xml_tagtab_t::generate (const char *s)
{
  ptr<xml_element_t> *e = _tab[s];
  if (!e) { return NULL; }
  else { return (*e)->clone (s); }
}

static void 
start_element_handler (void *pv, const char *name, const char **atts)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->start_element (name, atts);
}

static void
end_element_handler (void *pv, const char *name)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->end_element (name);
}

static void
character_data_handler (void *pv, const char *buf, int len)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->found_data (buf, len);
}


xml_req_parser_t::~xml_req_parser_t ()
{
  if (_xml_parser_init) {
    XML_ParserFree (_xml_parser);
  }
}

xml_tagtab_t xmltab;

void
xml_req_parser_t::start_element (const char *nm, const char **atts)
{
  ptr<xml_element_t> el = xmltab.generate (nm);
  if (el && active_el ()->add (el)) {
    push_el (el);
  } else {
    // handle error condition
  }
}

void
xml_req_parser_t::end_element (const char *nm)
{
  ptr<xml_element_t> el = pop_el ();
  if (!el || !active_el ()->is_a (nm)  || !el->close_tag ()) {
    // handle error condition
  }
}

void
xml_req_parser_t::found_data (const char *buf, int len)
{
  if (! active_el ()->add (buf, len)) {
    // handle error condition
  }
}

void
xml_req_parser_t::init (const char *encoding)
{
  assert (!_xml_parser_init);
  _xml_parser = XML_ParserCreate (encoding);
  XML_SetUserData (_xml_parser, this);
  XML_SetElementHandler (_xml_parser, start_element_handler, 
			 end_element_handler);
  XML_SetCharacterDataHandler (_xml_parser, character_data_handler);
  push_el (New refcounted<xml_top_level_t> ());
}

void
xml_req_parser_t::parse_guts ()
{
  const char *b;
  ssize_t sz;
  enum XML_Status xstat;

  do {
    sz = abuf->stream (&b);
    if (sz == ABUF_EOFCHAR) {
      xstat = XML_Parse (_xml_parser, NULL, 0, 1);
      finish_parse (0);
    } else if (sz >= 0) {
      xstat = XML_Parse (_xml_parser, b, sz, 0);
    } else {
      assert (sz == ABUF_WAITCHAR);
      // else, we're just waiting...
    }
  } while (sz >= 0);

}
