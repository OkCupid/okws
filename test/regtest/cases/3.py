#
# regression test 3 
#
#   test basic inclusion mechanism
#
#-----------------------------------------------------------------------

dct = { "v" : "var v",
        "js" : 'javascript:$("foo").bar',
        "exp" : "vv ${v} ${js} vv" }

parent1 = """ {$ include ("$[1]", %s) $} """  % dct
parent2 = """ 
{$ setl { file : "$[1]" } $}
{$ include (file, %s) $}
""" % dct

child = """
${v}
$${v}
${js}
$${js}
$${exp}
""" 

outcome ="""
%s
%s
%s
%s
vv %s %s vv""" % (dct["v"], dct["v"],
                  dct["js"], dct["js"],
                  dct["v"], dct["js"])

cases = [ { "filedata" : [ parent1, child ],
            "outcome" : outcome,
            "desc" : "straight-ahead file inclusion" },
          { "filedata" : [ parent2, child ],
            "outcome" : outcome,
            "desc" : "inclusion via filename expansion" } ]

