desc = "JSON dumps in arrays (21.py in OKWS 2.1)"

tot = 10

v = [ { "val" : "val-%d" % i } for i in range (0, tot) ]


filedata="""
{$ locals { v : %s } $}
{$ for (r, decorate (v)) {
      print r, "\\n"
   }
$}
""" % v

row="""{"val" : "val-%d", "odd" : %s, "first" : %s, "count" : %d, "last" : %s, "even" : %s, "iter" : %d}"""

def bool2str (b):
    r = "false"
    if b:
        r = "true"
    return r


def mkrow (i, tot):
    return row % (i,
                  bool2str (i % 2 == 1), 
                  bool2str (i == 0),
                  tot,
                  bool2str (i == tot - 1),
                  bool2str (i % 2 == 0),
                  i)
outcome = '\n'.join ( [ mkrow (i, tot) for i in range (0, tot) ] )

   
