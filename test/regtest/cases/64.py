
desc = "another test of append"

filedata = \
"""{$
   def bar () {
      locals { v : [ -1 ] }
      for (i, range (300)) {
         append (v, i);
      }
      print (v[231], "\n");
   }
   for (i, range (3)) {
     bar ();
   }
$}"""

outcome = '230 ' * 3
