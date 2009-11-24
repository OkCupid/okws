desc = "test assignments"

filedata = \
"""
{$
    def foo () { return 10; }
    locals { a, c, v : [], d : {} };
    a = d.fiz = v[1] = c = foo ();
    print (a, " ", d, " ", v, " ", c);
$}
"""

outcome = '10 {"fiz" : 10} [null, 10] 10'
