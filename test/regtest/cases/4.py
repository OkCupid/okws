desc = "simple function calls"

filedata = \
"""
{$
   def f (x) {
      locals { t }
      t = x + 1;
      return t;
   }

   def g (x) {
      locals { u }
      u = 2 * f(x);
      return u;
   }
   print (g(2), " ", g(10), "\n");
$}"""

outcome = "6 22"
