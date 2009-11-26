
desc = "more COW tests"

filedata = \
"""
{$
    def f(x) { return 2*x; }

    def g(x,i) {
        locals { v : [0, f(x), 2] }
        v[i] = "C";
        return v;
    }

    print (g(3,2), "\n", g(5,0), "\n");
$}
"""

outcome = '[0, 6, "C"] ["C", 10, 2]'

