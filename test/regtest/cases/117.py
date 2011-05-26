description = "compare lists"

l = [  [1, 2] ,
       [1, 3],
       [1],
       [1, -1],
       [3, 1, 2] ,
       [0, 101, 11],
       [ [1,2], 3, [1] ],
       [ [1,2], 3, [4,5] ],
       [ [1,2], 3, [4,4] ],
       [] 
       ]

comps = [ [0, 1],
          [1, 0],
          [2, 0],
          [2, 1],
          [2, 3],
          [0, 2],
          [0, 3],
          [3, 2],
          [4, 5],
          [5, 4],
          [5, 6],
          [6, 7],
          [7, 8],
          [8, 8],
          [6, 8],
          [9, 4],
          [5, 9 ] ]
        

filedata = """
{$
    locals { l : %(l)s, comps : %(comps)s, res : [] }
    for (c, comps) {
       append (res, l[c[0]] < l[c[1]],
                    l[c[0]] > l[c[1]],
                    l[c[0]] <= l[c[1]], 
                    l[c[0]] >= l[c[1]]);
    }
    print (join (" ", res));
$}""" % { "l" : l, "comps" : comps }

res = []

for c in comps:
    res += [ l[c[0]] < l[c[1]] ,
             l[c[0]] > l[c[1]] ,
             l[c[0]] <= l[c[1]], 
             l[c[0]] >= l[c[1]] ]

raw = " ".join ([str (s) for s in res])
outcome = raw.lower ()
