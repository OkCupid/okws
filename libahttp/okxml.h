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

#ifndef _LIBAHTTP_OKXML_H
#define _LIBAHTTP_OKXML_H

#include <async.h>
#include <ihash.h>
#include <ctype.h>
#include "aparse.h"
#include "okxml_data.h"

class xml_req_parser_t : public async_parser_t {
public:
  xml_req_parser_t (abuf_src *s) : async_parser_t (s) {}
  xml_req_parser_t (abuf_t *a) : async_parser_t (a) {}
private:
  virtual void parse_guts () { parse_guts_T (); }
  void parse_guts_T (CLOSURE);

};

#endif /* _LIBAHTTP_OKXML_H */
