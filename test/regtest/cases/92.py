description = "found this crasher in failed !might_block() assertion"

filedata = [
"""
{$
   def foo (do_include) {
      if (do_include) {
         include ("$[1]");
      } else {
         load ("$[1]");
      }
   }
   print ("nothing: ");
   foo (true);
   print ("nothing: ");
   foo (false);
   print ("nothing: ");
   foo (* false *);
   print ("something: ");
   foo (* true *);
$}""",
"""blah blah blah"""]

outcome = "nothing: nothing: nothing: something: blah blah blah"
