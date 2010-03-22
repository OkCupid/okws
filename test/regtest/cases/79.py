
description = "try out bind(), unbind() and lookup()"

filedata = """
{$
   locals { foo : 10, v : [ 1, 2, 3 ], x : [] }
   bind ("foo", 20, "l");
   bind ("x", v, "l");
   x[0] = 100;
   print (foo, " " , v[0], " ", v[1], " ");
   unbind ("v", "l");
   print (v, x[0], " ", lookup("x")[1], " ");
   lookup("x")[1] = 2000;
   print (x[1])
$}"""

outcome = "20 100 2 100 2 2000"

