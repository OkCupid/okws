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
test_keys = [ ("x", ["false", "false", "hi", "hi", "hi"]),
              ("y.k1", ["false", "false", "key 1", "key 1", "key 1"]),
              ("y.k4", ["true", "true", "", "", "xx"]),
              ("z", ["true", "true", "", "", "xx"]),
              ("e", ["false", "true", "", "", ""]) ]

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
isnull(y.k2): false
!y.k2: false
isnull(y.k3): false
!y.k3: true
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

cases = [ { "filedata" : filedata_fmt % "locals",
            "desc" : desc_fmt % "locals",
            "outcome" : outcome },
          { "filedata" : filedata_fmt % "globals",
            "desc" : desc_fmt % "globals",
            "outcome" : outcome } ]

#-----------------------------------------------------------------------
