desc = "test that define'd lambda can include other files..."

filedata = [
"""
{$
    locals { foo : ={ for (i, range (3)) { include ("$[1]")} } }
    locals { bar : ={{ yo dog {% for (i, range (2)) { include ("$[2]") } %} }} }
    print (foo, "\n", bar);
$}
""",
"chuck ",
"vest "
]

outcome = "chuck " * 3 + "yo dog " + "vest " * 2
