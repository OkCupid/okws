
desc = "forloop with functional decomposition (mainly 8.py from OKWS 2.1)"

items =  [ "dog", "cat", "mouse", "bird", "turtle", "fish", "dolphin" ]
ditems = [ { "val" : s } for s in items ]

d = { "lists" : [ ditems[0:i] for i in range (0, len(items)) ] }

caller = "{$ locals %s\n" % d + \
    '\n'.join (['include ("$[1]", { list : lists[%d] } )' % i \
                    for i in range (0, len (items)) ]) + "$}"

callee = """
{$ 
   locals { first : true } 
   for (r, list) {
      if (first) { first = false; }
      else { print ", " }
      print (r.val)
   } empty {{ emptiness! }}
   print "\n";
$}
"""

def listfn (l):
    r = ""
    if (len (l) == 0):
        r = "emptiness!"
    else:
        r = ", ".join (l)
    return r

outcome = '\n'.join ([ listfn (items[0:i]) for i in range (0, len (items)) ])
filedata =  [ caller, callee ]
                



