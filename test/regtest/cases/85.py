
import copy

description = "test sort"

arr = [ 10, 3, 44, 15, 40 , -10, -1000, 0, 0, 3000]


filedata = """{$
    locals { v : %(arr)s, v2 : [] }
    def rcmp (a, b) { return b - a; }
    v2 = sort (v, cmp);
    v3 = sort (v, rcmp);
    print (v2, " ", v3);
$}""" % { "arr" : arr }

a1 = copy.copy (arr)
a1.sort ()

a2 = copy.copy (arr)
a2.sort ()
a2.reverse ()

outcome = str (a1) + " " + str (a2)



