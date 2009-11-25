
desc = "simple test of for loops and shel"

filedata = \
"""{$
    locals { lst : [ "hi", "x", "b" ,"c", "ddddd" ] }
    for (i, lst) {
	  print "X ${i}\n";
    }
$}"""

outcome = "X hi X x X b X c X ddddd"
