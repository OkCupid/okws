// -*-c++-*-
/* $Id: err.C 2838 2007-04-30 19:04:58Z max $ */

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


#include "resp.h"
#include "ahttp.h"

strbuf
http_error_t::make_body (int n, const str &si, const str &aux)
{
  strbuf b;
  str ldesc;
  const str sdesc = http_status.get_desc (n, &ldesc);
  b << "<html>\n"
    << " <head>\n"
    << "  <title>" << n << " " << sdesc << "</title>\n"
    << " </head>\n"
    << " <body>\n"
    << " <h1>Error " << n << " " << sdesc << "</h1><br><br>\n"
    ;
  if (n == HTTP_NOT_FOUND && aux) {
    b << "The file <tt>" << aux 
      << "</tt> was not found on this server.<br><br>\n\n";
  }
  b << "  <hr>\n"
    << "  <i>" << si << "</i>\n"
    << " <br>\n"
    << " </body>\n"
    << "</html>\n"
    ;
  return b;
}
