
desc = "basic test of lambda closures"

filedata = \
"""
{$
     def make_adder (x) {
        locals { r : 
                 lambda (y) { 
                    return x + y; 
                } 
           }
        return r;
     }
     locals { fn : make_adder (10) }
     locals { v  : fn (3) }
     print (v, "\n");
$}
"""

outcome = "13"
