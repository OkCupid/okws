
desc = "test legacy support of switch statements"

filedata=[ """
{$ globals { v : [ "v0v0v0", "v1v1v1", "v2v2v2", "v3v3v3" ],
         bar : "BARs" }
   include ("$[1]", { foo : "f1" })
   include ("$[1]", { foo : "f2" })
   include ("$[1]", { foo : "f3" })
   include ("$[1]")
$}
""",
"""
{$ 
   switch (foo) { 
     case ("f1") { include ("$[2]", { "value" : v[1] } ) }
     case ("f2") { include ("$[2]", { "value" : "${v[2]}xx${bar}xx" } ) }
     case ("f3") { include ("$[2]", { "value" : v[3] } ) }
     case {        include ("$[2]", { "value" : "null" } ) } } $}
""",
"""
${value}
"""]


outcome="""
v1v1v1
v2v2v2xxBARsxx
v3v3v3
null
"""
