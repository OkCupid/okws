desc = "basic function calls and recursion"

filedata = \
"""
{$
    def fact (x) {
       if (x <= 0) { 
           return 1; 
       }
       else { 
          locals { y }
          y = x * fact (x - 1)
          return y;
        }
    }

    def fact2 (x) {
      if (x <= 0) { return 1; }
      locals { y }
      y = x * fact2 (x - 1)
      return y;
    }

    locals { t, u }
    t = fact (8);
    u = fact2 (8);
    print (t, "\n", u, "\n");
$}
"""

outcome = "40320 " * 2
