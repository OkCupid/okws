description = "check the shuffle and randsel filter"

filedata = """
{$ 
   locals { v : range (0,10) }
   def sum (v) {
     locals { out : 0 }
     for (x, v) { out = out + x; }
     return out;
   }
   shuffle (v);
   print (sum(v));
   print (" ");
   locals { p : randsel (v) }
   locals { z : p[0] - v[p[1]] }
   print (z);
$}"""

outcome = "45 0"
