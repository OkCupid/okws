#
# Regtest 1
#
#   isnull(x), !x, x|default, etc...
#
#-----------------------------------------------------------------------

#
# a list of the functions and features to test
#
test_fns =  [ "isnull(%s)", "!%s", "%s|default" , '%s|default("")', 
              '%s|default("xx")' ]

#
# a list of the keys to test, followed by the expected results
# when the keys are run through the above test functions.
#
test_keys = [ ("x", ["0", "0", "hi", "hi", "hi"]),
              ("y.k1", ["0", "0,", "key 1", "key 1", "key 1"]),
              ("y.k4", ["1", "1", "", "", "xx"]),
              ("z", ["1", "1", "", "", "xx"]),
              ("e", ["0", "1", "", "", ""]) ]

#-----------------------------------------------------------------------

def in_cases (k):
    v = []
    for f in test_fns:
        v += [ (f + ": ${" + f + "}") % (k, k) ]
    return '\n'.join (v)

teststr = '\n'.join ([ in_cases (k[0]) for k in test_keys ]) + """
isnull(y.k2): ${isnull (y.k2)}
!y.k2: ${!y.k2}
isnull(y.k3): ${isnull (y.k3)}
!y.k3: ${!y.k3}
"""

#-----------------------------------------------------------------------

def out_cases (k):
    v = []
    i = 0
    for f in test_fns:
        v += [ ("%s: %s" % (f, k[1][i])) % k[0] ]
        i += 1
    return '\n'.join (v)

outcome = '\n'.join ([ out_cases (k) for k in test_keys ]) + """
isnull(y.k2): 0
!y.k2: 0
isnull(y.k3): 0
!y.k2: 1
"""

#-----------------------------------------------------------------------

desc_fmt = "isnull(), !x, default with %s"

filedata_fmt = """
{$ %s { x : "hi",
          y : { k1 : "key 1", 
                k2 : [ "enie", "menie", "minie", "moe"],
                k3 : [] },
          e : "" } $}
""" + teststr


#-----------------------------------------------------------------------

cases = [ { "filedata" : filedata_fmt % "setl",
            "desc" : desc_fmt % "setl",
            "outcome" : outcome },
          { "filedata" : filedata_fmt % "set",
            "desc" : desc_fmt % "set",
            "outcome" : outcome } ]

#-----------------------------------------------------------------------
