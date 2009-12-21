#
# Regtest 2
#
# forloops, vectors, etc.
#
#-----------------------------------------------------------------------

d = { "v1" : [ 20 * x for x in range (0,10) ],
      "v2" : [ { "x" : "x @ %d" % i, "y" : i } for i in range (0,5) ],
      "v3" : [ { "val" : "val-%d-val" % i } for i in range (0, 4) ],
      "v4" : [], 
      "v5" : [ "singleton" ],
      "table" : [ { "val" : r, 
                    "col" : [ {"val" : c} for c in range (0,6) ] }
                  for r in range (0, 7) ]
}

#-----------------------------------------------------------------------

def filedata_fn (func):
    return \
"""

{$
   %s %s

   for i, v1 { print i, " " } print "\\n"
   for i, v1 {{ ${i} }} print "\\n"
   for (i, v1) { print (i, " ") } print "\\n"
   for (i, v1) {{ ${i} }} print "\\n"
  
   for (i, v2) {{${i.y}-}} print "\\n"
   for (i, decorate (v2)) {{${i.x} ${i.y}; }} print "\\n"
   
   for (i, decorate (v3)) {{
     val=${i.val} last=${i.last} iter=${i.iter} odd=${i.odd} count=${i.count}
   }} 
   print "\\n"

   for (i, v4) { print "don't show me and don't crash !" }
   for (i, v4) { print i } empty {{ empty! }}  print "\\n"
   for (i, v9) { print i } empty {{ empty! }}  print "\\n"

   for (r, table) {
     for (c, r.col) {
        print (r.val, "," , c.val, " ")
     }
     print "\\n"
  }
$}
""" % (func, d)

#-----------------------------------------------------------------------

def smush (v):
    return ''.join (v)

#-----------------------------------------------------------------------

def bool2str (b):
    r = "false"
    if b:
        r = "true"
    return r

#-----------------------------------------------------------------------

test1 = smush ([ "%d " % i for i in d["v1"] ])
test2 = smush ([ "%d-" % e["y"] for e in d["v2"] ])
test3 = smush ([ "%s %d; " % ( e["x"], e["y"] ) for e in d["v2"] ])

v = []
inv = d["v3"]
for i in range (0, len (inv)):
    v += [ "val=%s last=%s iter=%d odd=%s count=%d" % \
               (inv[i]["val"], 
                bool2str (i == len (inv) - 1), 
                i, 
                bool2str (i % 2 == 1), 
                len (inv)) ]
test4 = '\n'.join (v)

test5 = "empty!"

v = []
for r in d["table"]:
    for c in r["col"]:
        v += [ "%d,%d " % ( r["val"], c["val"] ) ]
    v += [ "\n" ]

test6 = smush (v)

tests = [ test1, test1, test1, test1,
          test2, 
          test3, 
          test4,
          test5, 
          test5,
          test6 ]

outcome = '\n'.join (tests)

#-----------------------------------------------------------------------

desc_fmt = "forloops/vectors with %s"

#-----------------------------------------------------------------------

def make_testcase (func):
    return { "filedata" : filedata_fn (func),
             "desc" : desc_fmt % func,
             "outcome" : outcome } 

#-----------------------------------------------------------------------

cases = [ make_testcase (f) for f in [ "locals", "globals" ] ]
