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


#include "resp.h"
#include "ahttp.h"

strbuf
http_error_t::make_body (int n, const str &si, const str &aux)
{
  strbuf b;
  str ldesc;
  const str sdesc = http_status.get_desc (n, &ldesc);

  /*<pub>
    print (b) <<EOF;
<html>
 <head>
  <title>@{n} @{sdesc}</title>
 </head>
 <body>
  <h1>Error @{n} @{sdesc}</h1><br><br>
EOF
      </pub>*/
  if (n == HTTP_NOT_FOUND && aux) {
    /*<pub>
      print (b) <<EOF;
The file <tt>@{aux}</tt> was not found on this server.<br><br>
EOF
      </pub>*/
  }
  /*<pub>
    print (b) <<EOF;
<hr>
  <i>@{si}</i>
 <br>
 </body>
</html>
EOF
   </pub>*/
  
  return b;
}
