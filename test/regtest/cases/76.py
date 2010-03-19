
description = "check dictionaries & lists work as references"

filedata = """
{$
   locals { l : [1, 2, 3 ],
            d : { a : 1 , b : 2 } }

   locals { r2 : [ d ], 
            r1 : null,
            v  : [ l ],
            c  : d }

   r1 = { x : l };

   r1.x[0] = 100;
   r2[0].b = 200;

   print (l[0], " " , d.b, " ");

   v[0][0] = 1000;
   print (l[0], " ", r1.x[0], " ");
 
   c.b = 2000;
   print (d.b, " ", r2[0].b);

$}
"""
outcome = "100 200 1000 1000 2000 2000"
