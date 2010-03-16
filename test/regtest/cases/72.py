description = "test type()"

filedata = """
{$
   locals { s : "foo",
            l : [ 1, 2, 3 ],
            d : { a : 1 } }

   if (type (s) == "str") { print "A "; } 
   if (type (l) == "list") { print "B "; }
   if (type (d) == "dict") { print "C "; }
   if (s == "foo") { print "D "; }
$}
"""

outcome = "A B C D";

