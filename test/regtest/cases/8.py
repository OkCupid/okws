
items =  [ "dog", "cat", "mouse", "bird", "turtle", "fish", "dolphin" ]
ditems = [ { "val" : s } for s in items ]

d = { "lists" : [ ditems[0:i] for i in range (0, len(items)) ] }


caller = "{$ setl %s\n" % d + \
    '\n'.join (['include ("$[1]", { list : lists[%d] } )' % i \
                    for i in range (0, len (items)) ]) + "$}"

callee = """
{$ for r, list {

   if   (!r.first && r.last) { print " and " }
   elif (!r.first)           { print ", " }

   print (r.val)
} empty {{ emptiness! }}
$}
"""

def listfn (l):
    r = ""
    if (len (l) == 0):
        r = "emptiness!"
    else:
        for i in range (0, len (l)):
            if i != 0 and i == len (l) - 1:
                r += " and "
            elif i != 0:
                r += ", "
            r += l[i]
    return r

outcome = '\n'.join ([ listfn (items[0:i]) for i in range (0, len (items)) ])
filedata =  [ caller, callee ]
desc = "forloop with functional decomposition"
                



