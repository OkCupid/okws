description = "check floating math ops"

import math

def mkargs (tup):
    return ", ".join ([ str (x) for x in tup ])

ops = [ ("log", ( 244.33, ) ),
        ("exp", ( 3.23, ) ),
        ("sqrt" , ( 39.33, ) ),
        ("pow", (9.431, 5.323 ) ) ]

values = [ str (int (round (getattr(math, p[0]) (*p[1])))) for p in ops ]

pub = ", ".join ([ "%s (%s)" % (p[0], mkargs (p[1])) for p in ops])

filedata = """
{$ 
   locals { v : [ %s ] }
   for (x, v) {
      print (round (x));
      print (" ");
   }
$}""" % (pub)

outcome = " ".join (values)
