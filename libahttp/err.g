
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
  if (n == 500 || n == 404) {
/*<pub>
  print (b) <<EOF;

<html>

<head>
<title>Unnnnnh.</title>
<link rel="stylesheet" type="text/css" href="http://is0.okcupid.com/css/standard.css">
</head>

<body bgcolor = "#acc3ff" leftmargin = "0" topmargin = "0">

<!--- @{n} @{sdesc} --->

<table width = "720" cellspacing = "0" cellpadding = "0" border = "0" bgcolor = "#dddddd" background = "http://is0.okcupid.com/graphics/fr_topbar.gif">
 <tr>
  <td background = "http://is0.okcupid.com/graphics/fr_topbar.gif" height = "26">
   &nbsp;
  </td>
 </tr>
</table>

<table width = "720" height = "380" cellspacing = "0" cellpadding = "3" border = "0" bgcolor = "white" class = "maincolumn">
 <tr>
  <td align = "center">
   <font size = "6">
    Internal Server Error 500
   </font>
   <br>
   <font size = "4">
    Something has gone terribly wrong with our matching service.
   </font>
   <br><br>
   <img src = "http://is0.okcupid.com/graphics/girlsign.jpg">
  </td>
 </tr>
</table>

 <form action="/login" method="POST" name = "f">

<table width = "720" height = "50" cellspacing = "0" cellpadding = "3" border = "0" bgcolor = "#4f7bed">
 <tr>
  <td bgcolor = "#4f7bed" width = "300">
   &nbsp;
  </td>
  <td align = "right">
   <img src = "http://is0.okcupid.com/graphics/fr_signin.gif">
  </td>
  <td>
   <input type = "text" name = "username" size = "12" maxlength = "20">
  </td>
  <td align = "right">
   <img src = "http://is0.okcupid.com/graphics/fr_password.gif">
  </td>
  <td>
   <input type = "password" name = "password" size = "12" maxlength = "20">
  </td>
  <td>
   <input type = "image" src = "http://is0.okcupid.com/graphics/submit_button_go.gif" name = "login" value = " Go ">
  </td>
 </tr>
</table>

</form>

<table width = "720" cellspacing = "0" cellpadding = "3" border = "0" bgcolor = "#7199ff">
 <tr>
  <td bgcolor = "#7199ff" height = "5">
   &nbsp;
  </td>
 </tr>
</table>

<table width = "730" cellspacing = "0" cellpadding = "0" border = "0" bgcolor = "white">
 <tr>
  <td bgcolor = "white" width = "720" align = "right" valign = "middle" height = "22">
   OkCupid. &nbsp;&copy; 2003.
   <img src = "http://is0.okcupid.com/graphics/fr_rainbow_l.gif">
  </td>
  <td width = "10" bgcolor = "#acc3ff" align = "left" valign = "middle" height = "22">
   <img src = "http://is0.okcupid.com/graphics/fr_rainbow_r.gif">   
  </td>
 </tr>
</table>


</body>
</html>

  
EOF
</pub>*/
  }
  else {
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
  }
  
  return b;
}
