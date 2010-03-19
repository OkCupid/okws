
description = "check dictionaries & lists work as references"

filedata = """
{$
   locals { l : [1, 2, 3 ],
            d : { a : 1 , b : 2 } }

   locals { r2 : [ d ], r1 : null }
   r1 = { x : l };

   r1.x[0] = 100;
   r2[0].b = 200;

   print (l[0], " " , d.b);
$}
"""
outcome = "100 200"
