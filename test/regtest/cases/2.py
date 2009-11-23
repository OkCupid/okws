
desc = "a test of forloops -- that the input list is eval'ed properly"

filedata = \
"""
{$
    locals { x : "x1",
             y : "y2",
             z : "z3" }
    for (i, [ "x ${x}", "y ${y}", "z ${z}" ] ) {
	  print "xxx ${i}\n";
    }
$}"""

outcome = "xxx x x1 xxx y y2 xxx z z3"
