desc = "check that list modification is working properly in for loops"

filedata = \
"""{$
    for (j, [0,1]) {
       locals { v  : [ [ 1, "a" ], [ 2, "b"] , [ 3, "c" ] ] }
       for (p, v) {
          p[1] = "X" + p[1] + "x" + p[0]
        }
        for (p, v) {
           print (p[1], "\n");
        }
     }
$}"""

outcome = "Xax1 Xbx2 Xcx3 " * 2
