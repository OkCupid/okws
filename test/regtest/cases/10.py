
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
     locals { fn : make_adder (10); }
     print (fn (3));
$}
"""

outcome = "13"
