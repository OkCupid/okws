
desc = "test COW mechanisms for arrays."


filedata = \
"""
{$
    def smush (v) {
        locals { r : "" };
        r = "" + v[0] + v[1] + v[2][0] + v[3].d;
        return r;
    }

    def tester (i, x) {
       locals { v : [ 1, "b", ["c"], {"d" : 1 } ] }
       v[i] = x;
       return v;
    }

    print (smush (tester(0, 0)), "\n");
    print (smush (tester(1, "B")), "\n");
    print (smush (tester(2, ["C"])), "\n");
    print (smush (tester(3, {"d" : 2})), "\n");
    print (smush (tester(0,0)), "\n");
$}
"""

outcome = '\n'.join ([ "0bc1", "1Bc1", "1bC1", "1bc2", "0bc1"])
