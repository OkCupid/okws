
// -*-c++-*-
/* $Id$ */

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
