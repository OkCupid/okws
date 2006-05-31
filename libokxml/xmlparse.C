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


#include "okxmlparse.h"
#include "config.h"
#ifdef HAVE_EXPAT

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
  I(fault);
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
  if (el) {
    if (active_el ()->add (el)) {
      push_el (el);
    } else {
      strbuf b;
      b << "cannot add item: " << nm;
      parse_error (XML_PARSE_BAD_NESTING, b);
    }
  } else {
    parse_error (XML_PARSE_UNKNOWN_ELEMENT, nm);
  }
}

void
xml_req_parser_t::end_element (const char *nm)
{
  ptr<xml_element_t> el = pop_el ();
  if (!el) {
    parse_error (XML_PARSE_UNBALANCED, NULL);
  } else if (!el->is_a (nm)) {
    str m;
    if (el->name ()) {
      strbuf b;
      b << "Unexpected tag '</" << el->name () << "'>; got tag '</" 
	<< nm  << ">'";
      m = b;
    } else {
      m = "Didn't expected a close tag";
    }
    parse_error (XML_PARSE_UNMATCHED, m);
  } else if (!el->close_tag ()) {
    strbuf b;
    b << "tag type '" << nm << "'";
    parse_error (XML_PARSE_CLOSE_ERROR, b);
  }
}

void
xml_req_parser_t::found_data (const char *buf, int len)
{
  xml_element_t *el = active_el ();
  if (el->gets_char_data ()) {
    if (!el->add (buf, len)) {
      strbuf b;
      str tmp (buf, len);
      b << "bad character data for element of type '" << el->name () << "'; "
	<< "data is: '" << tmp << "'";
      parse_error (XML_PARSE_BAD_CHARDATA, b);
    }
  } else if (has_non_ws (buf, len)) {
    strbuf b;
    str m;
    if (el->name ()) {
      b << "for element of type '" << el->name () << "'; ";
    }
    str tmp (buf, len);
    b << "data is: '" << tmp << "'";
    m = b;
    parse_error (XML_PARSE_UNEXPECTED_CHARDATA, m);
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
  push_el (_top_level);
}

void
xml_req_parser_t::parse_guts ()
{
  const char *b;
  ssize_t sz;
  enum XML_Status xstat;
  bool ok = true;

  do {
    sz = abuf->stream (&b);
    xstat = XML_STATUS_OK;
    if (sz == ABUF_EOFCHAR) {
      warn << "at EOF!\n";
      xstat = XML_Parse (_xml_parser, NULL, 0, 1);
      finish_parse (0);
    } else if (sz >= 0) {
      str xx (b, sz);
      warn << "parse this: " << xx << "\n";
      xstat = XML_Parse (_xml_parser, b, sz, 0);
    } else {
      warn << "just waiting!\n";
      assert (sz == ABUF_WAITCHAR);
      // else, we're just waiting...
    }

    if (_status != XML_PARSE_OK) {
      xstat = XML_STATUS_ERROR;
    } else if (xstat == XML_STATUS_ERROR) {
      _status = XML_PARSE_EXPAT_ERROR;
      strbuf bf;
      bf << "Error code (" << XML_GetErrorCode (_xml_parser) << "): "
	 <<  XML_ErrorString (XML_GetErrorCode (_xml_parser));
      _err_msg = bf;
    }

    if (xstat == XML_STATUS_ERROR) {
      ok = false;
      finish_parse (int (_status));
    }
  } while (sz >= 0 && ok);

}

str
xml_req_parser_t::errmsg () const
{
  static char *msgs[] = { NULL,
			  "Bad tag nesting",
			  "Unknown element",
			  "Unexpected character data",
			  "Unbalanced tags", 
			  "Unmatched tags",
			  "Close tag error",
			  "Expat error",
			  "Bad character data" };
			  

  str s;
  if (_status != XML_PARSE_OK) {
    strbuf b;
    b << msgs[_status];
    if (_err_msg) {
      b << ": " << _err_msg;
    }
    s = b;
  }
  return s;
}

void
xml_req_parser_t::parse_error (xml_parse_status_t s, str m)
{
  _status = s;
  _err_msg = m;
  XML_StopParser (_xml_parser, 0);
}



#endif /* HAVE_EXPAT */
