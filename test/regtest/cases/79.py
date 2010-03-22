
description = "try out the bind() and unbind()"

filedata = """
{$
   locals { foo : 10, v : [ 1, 2, 3 ], x : [] }
   bind ("foo", 20, "l");
   bind ("x", v, "l");
   x[0] = 100;
   print (foo, " " , v[0], " ", v[1], " ");
   unbind ("v", "l");
   print (v, x[0]);
$}"""

outcome = "20 100 2 100"

