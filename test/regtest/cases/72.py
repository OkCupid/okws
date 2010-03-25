description = "test type() + library namespace binding"

filedata = """
{$
   locals { s : "foo",
            l : [ 1, 2, 3 ],
            d : { a : 1 } }

   if (rfn3.type (s) == "str") { print "A "; } 
   if (rfn3.type (l) == "list") { print "B "; }
   if (rfn3.type (d) == "dict") { print "C "; }
   if (s == "foo") { print "D "; }
$}
"""

outcome = "A B C D";

