
description = "test while() v. continue and break"

filedata = """
{$
   locals { a : 0, b : 0 }
   while (a < 10) {
      a = a + 1;
      if (a > 5) { continue; }
      b = b + 1;
   }
   print (a, " ", b, " ");

   a = b = 0;
   while (a < 10) {
      a = a + 1;
      if (a > 5) { break; }
      b = b + 1;
   }
   print (a, " ", b);
$}
"""

outcome = "10 5 6 5"





