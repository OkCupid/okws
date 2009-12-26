
desc = "another test of append"

filedata = \
"""{$
   def bar () {
      locals { v : ["foo"] }
      append (v, 1);
      append (v, 2, 3, 4);
      append (v, 5, 6);
      print (v, "\n");
   }
   for (i, range (3)) {
     bar ();
   }
$}"""

outcome = '["foo", 1, 2, 3, 4, 5, 6] ' * 3
