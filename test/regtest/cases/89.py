description = "testing weird problems in returning arrays -- r[foo]"

filedata = """
{$
   def foo () {
     locals { v : [ 10, 20, 30 ] }
     return v;
   }
   locals { r : foo () }
   print (r[1]);
$}"""

outcome = "20"

