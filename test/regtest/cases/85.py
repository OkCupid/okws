
import copy

description = "test sort"

arr = [ 10, 3, 44, 15, 40 , -10, -1000, 0, 0, 3000]
arr2 = [ -10.2, 4.33, 1.999, -399.22, -10000.1001, 10.9, 3.922, 59.01, -33.11, 0.3, 0.2, 0.1, 0.001, -0.001, -0.2, -0.4, -0.3, -0.222 ]
arr3 = [ { "key" : k } for k in arr2 ]


filedata = """{$
    locals { v : %(arr)s, v2 : [], w : %(arr2)s, u : %(arr3)s, l }
    def rcmp (a, b) { return b - a; }
    v2 = sort (v, cmp);
    v3 = sort (v, rcmp);
    v4 = sort (w);
    l = lambda (a,b) { 
       locals { diff : b.key - a.key };
       return cmp_float (diff);
    } ;
    v5 = sort (u, l);
    v6 = sort2 (u, lambda (x) {  return (0 - x.key); } ) ;
    print (v2, " ", v3, " ", v4, " ", v5, " ", v6);
$}""" % { "arr" : arr, "arr2" : arr2, "arr3" : arr3 }

a1 = copy.copy (arr)
a1.sort ()

a2 = copy.copy (arr)
a2.sort ()
a2.reverse ()

a3 = copy.copy (arr2)
a3.sort ()

a4 = copy.copy (arr3)
a4.sort (lambda x,y: cmp (y["key"], x["key"]) )

a3s = str([ "%.12g" % x for x in a3 ]).replace("'","")
a4s = str ([ { "key" : "%.12g" % x["key"] } for x in a4 ]).replace("'","")
a4s = a4s.replace ("key", '"key"').replace ('": ', '" : ')


outcome = " ".join ([str (s) for s in [ a1, a2, a3s, a4s, a4s ] ])



