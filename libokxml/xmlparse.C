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
#include "okwsconf.h"
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
    _tab.insert (e->xml_typename (), e);    \
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
  I(method_response);
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
  else { return (*e)->generate (s); }
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

static void
cdata_start_handler (void *pv)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->start_cdata ();
}

static void
cdata_end_handler (void *pv)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->end_cdata ();
}

xml_req_parser_t::~xml_req_parser_t ()
{
  if (_xml_parser_init) {
    XML_ParserFree (_xml_parser);
  }
}

xml_tagtab_t xmltab;

ptr<xml_element_t>
xml_req_parser_t::generate (const char *nm, const char **atts)
{
  return xmltab.generate (nm);
}

void
xml_req_parser_t::start_element (const char *nm, const char **atts)
{
  ptr<xml_element_t> el = generate (nm, atts);
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
    if (el->xml_typename ()) {
      strbuf b;
      b << "Unexpected tag '</" << el->xml_typename () << "'>; got tag '</" 
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
xml_req_parser_t::start_cdata (void)
{
  xml_element_t *el = active_el ();
  if (!el->start_cdata ()) {
    strbuf b;
    str m;
    if (el->xml_typename ()) {
      b << "for element of type '" << el->xml_typename () << "'; ";
    }
    m = b;
    parse_error (XML_PARSE_UNEXPECTED_CDATA, m);
  }
}

void
xml_req_parser_t::end_cdata (void)
{
  xml_element_t *el = active_el ();
  if (!el->end_cdata ()) {
    strbuf b;
    str m;
    if (el->xml_typename ()) {
      b << "for element of type '" << el->xml_typename () << "'; ";
    }
    m = b;
    parse_error (XML_PARSE_BAD_CDATA, m);
  }
}

void
xml_req_parser_t::found_data (const char *buf, int len)
{
  xml_element_t *el = active_el ();
  if (el->gets_data ()) {
    if (!el->add (buf, len)) {
      strbuf b;
      str tmp (buf, len);
      b << "bad character data for element of type '" 
        << el->xml_typename () << "'; "
	<< "data is: '" << tmp << "'";
      parse_error (XML_PARSE_BAD_CHARDATA, b);
    }
  } else if (has_non_ws (buf, len)) {
    strbuf b;
    str m;
    if (el->xml_typename ()) {
      b << "for element of type '" << el->xml_typename () << "'; ";
    }
    str tmp (buf, len);
    b << "unexpected character data is: '" << tmp << "'";
    m = b;
    parse_error (XML_PARSE_UNEXPECTED_CHARDATA, m);
  } else {
    // just spaces; ignore it
  }
}

void
xml_req_parser_t::init (const char *encoding)
{
  assert (!_xml_parser_init);
  v_init ();
  _xml_parser_init = true;
  _xml_parser = XML_ParserCreate (encoding);
  XML_SetUserData (_xml_parser, this);
  XML_SetElementHandler (_xml_parser, start_element_handler, 
			 end_element_handler);
  XML_SetCharacterDataHandler (_xml_parser, character_data_handler);
  XML_SetCdataSectionHandler (_xml_parser, cdata_start_handler,
			      cdata_end_handler);
}

void
xml_req_parser_t::v_init ()
{
  _top_level = New refcounted<xml_top_level_t> ();
  push_el (_top_level);
}

void
xml_req_parser_t::parse_guts ()
{
  const char *b;
  ssize_t sz;
  enum XML_Status xstat;
  bool done = false;

  do {
    sz = abuf->stream (&b);
    xstat = XML_STATUS_OK;
    if (sz == ABUF_EOFCHAR) {
      xstat = XML_Parse (_xml_parser, NULL, 0, 1);
      done = true;
    } else if (sz >= 0) {
      /*
       * debug..
       *
      str xx (b, sz);
      warn << "parse this: " << xx << "\n";
      */
      xstat = XML_Parse (_xml_parser, b, sz, 0);
    } else {
      assert (sz == ABUF_WAITCHAR);
      // else, we're just waiting...
    }

    if (_status != XML_PARSE_OK) {
      xstat = XML_STATUS_ERROR;
    } else if (xstat == XML_STATUS_ERROR) {
      _status = XML_GetErrorCode (_xml_parser);
      strbuf bf;
      bf << "Expat parse error: " 
	 <<  XML_ErrorString (XML_GetErrorCode (_xml_parser));
      _err_msg = bf;
    }

    if (xstat == XML_STATUS_ERROR) 
      done = true;

  } while (sz >= 0 && !done);

  if (done)
    // still finish with HTTP_OK, signifying that we can send back
    // an HTTP 200 to the client; the body, however, will contain
    // the fault status showing the parse error
    finish_parse (HTTP_OK);

}

str
xml_req_parser_t::errmsg () const
{
  return _err_msg;
}

void
xml_req_parser_t::parse_error (int s, str m)
{
  warn << "parse error: " << m << "\n";
  _status = s;
  _err_msg = m;
  cancel ();
}

void
xml_req_parser_t::cancel ()
{
  XML_StopParser (_xml_parser, 0);
}


#endif /* HAVE_EXPAT */
