<%ec (mform_1a, b) %>
<%cf
void
mform_1a::foo ()
{
  for (i = 0; i < 10; i++) {
    b << "<td>" << i << "</td>\n";
  }
}

void 
mform_1a::bar ()
{
  for (int i = 0; i < 20; i++) {
    b << "<tr>";
    foo ();
    b << "</tr>\n";
  }
}
%>

<%m%>

<%v i, j, fvar; %>

<%c str fvar = "< resolution of FVAR goes here >"; %>

<!-- main code will be put into mform_1a::ec_main (); -->

<html>
<head>
<title>Trying Out Embedded C Files</title>
</head>
<body bgcolor="white">
<%g ct_include (b, "/home/u1/max/a.html", { X => "Y" }); %>
Blah blah balh
<table>
 <%c for (int i = 0; i < 20; i++) { %>
   <tr>
    <%c for (int j = 0; i < 10; j++) { %>
     <td>Table Element: ${i},${j}</td>
     <%g ct_include (b, "/home/u1/max/b.html", { X => "${i}", Y => "${j}" }); 
      %>
    <%c } %>
   </tr>
 <%c } %>
</table>
<!-- Run-time include below: -->
<%g include (pub, b, "/home/u1/max/c.html",  { Y => 123 }); %>
<%/m%>

<%c
void
mform_1a::ec_main2 ()
{
  for (int i = 0; i < 12; i++) {
%>
   <td><img src=${i}.jpg></td>
<%cf } 
}
%>
  
