<%ec (mform_1a, b) %>
<%m%>
<%v i, j; %>
<html>
<!--#set ( { fvar => "FUCKED VARIABLE" } ); -->
<!-- A small comment -->
<%c for (int i = 0; i < 20; i++) { %>
 <tr>
  <%c for (int j = 0; i < 10; j++) { %>
   <td>Table Element: ${i},${j}
    <%g ct_include (b, "/home/u1/max/b.html", { X => "${i}", Y => "${j}" }); %>
    <%g ct_include (b, "/home/u1/max/d.html", { X => "${i}", Y => "${j}" }); %>
    </td>
  <%c } %>
  </tr>
<%c } %>
</table>
<!-- Run-time include below: -->
<%/m%>
<%c
void foo () {
%>
   <%g vars a, b, c; %>
   bartime
<%c } %>
<%uv i, j; %>
<%cf
void bar ()
{
  b << "baz";
}
%>
