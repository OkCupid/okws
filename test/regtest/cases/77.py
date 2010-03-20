
description = "test for() v. continue"

filedata = """
{$
   locals { a : 0, b : 0 }
   for (i, range (0, 10)) {
      a = a + 1;
      if (i >= 5) { continue; }
      b = b + 1;
   }
   print (a, " ", b);
$}
"""

outcome = "10 5"





