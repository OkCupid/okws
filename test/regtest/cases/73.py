
description = "check len() on cows"

filedata = """
{$
   locals { x : [ 1, 2, 3],
            y : [],
            z : [ "bar", "jam", "jiggles", "wiggle" ],
            d : { a : 1, b : 2, c: 3 } }

   print (len (x), " ", len (y), " ", len (z), " ", len (d));
$}
"""

outcome = "3 0 4 3"
