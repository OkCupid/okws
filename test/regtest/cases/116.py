description = "test sleep and shotgun"

filedata = """
{$
    locals { v : [ lambda () { sleep (* 1 *); return 1; },
                   lambda () { sleep (* 0, 500 *); return 2; },
                   lambda () { sleep (* 2, 1 *); return 3; } ],
             out : [] }
    out = shotgun(* v *);
    print (out);
$}"""

outcome = repr ([ 1, 2, 3])
