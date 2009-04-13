#
# regression test 4
#
# shell strings
#
#-----------------------------------------------------------------------


def filedata_fn (func):
    return """
{$ %s { x : "base x",
        y : "base y" } $}
{$ %s { x : "derived x: ${x}; ${y}",
        y : "derived y: ${y}; ${x}" } $}
${x}
$${x}
${y}
$${y}
""" % (func, func)

outcomes = { "set" : """
derived x: base x; base y
derived x: base x; base y
derived y: base y; base x
derived y: base y; base x
""", 
             "setl" : """
derived x: base x; base y
derived x: derived x: base x; base y; derived y: base y; base x
derived y: base y; base x
derived y: derived y: base y; base x; derived x: base x; base y
""" } 

desc_fmt = "shell string expansion (via %s)"

cases = [ { "filedata" : filedata_fn (f),
            "desc" : desc_fmt % f,
            "outcome" : outcomes[f] } for f in [ "set", "setl" ] ]

