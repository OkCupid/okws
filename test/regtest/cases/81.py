
description = "test pop_front"

filedata = """
{$
    locals { x : ["a","b"] }
    locals { v : [x,1,2] }
    locals { y : pop_front (v) }
    y[0] = "c";
    print (v[0], " ", x[0]);
$}"""

outcome = "1 c"
