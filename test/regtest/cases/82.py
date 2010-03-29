
description = "test floaters"

filedata = """
{$
    locals { v : [ 1.11, 2.2222, 3.5, 3.9, 10.000 ] }
    for (x, v) {
      print (round (x), " ", ceil (x), " ", floor (x), " ");
    }

$}"""

outcome = "1 2 1 2 3 2 4 4 3 4 4 3 10 10 10"
