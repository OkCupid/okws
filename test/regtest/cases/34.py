
d = "test of the append() library function"

f ="""
{$
   locals { v  : [ 1, "hi" ] }
   locals { x  : [ 1, 2, 3, 4 ] }
   append (v, 3, { x : "y" }, "bye");
   append (v, 10, x);
   append (v[6], 20);

   print (v);
$}
"""

o = [1, "hi", 3, {"x" : "y"}, "bye", 10, [1, 2, 3, 4, 20] ]


case = { "filedata" : f,
         "desc" : d,
         "outcome_data": o }

# make sure it works multiple time and doesn't keep updating v
cases = [ case for i in range (0,3) ]

