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
${y}
""" % (func, func)

outcome = """
derived x: base x; base y
derived y: base y; derived x: base x; base y
"""

desc_fmt = "shell string expansion (via %s)"

cases = [ { "filedata" : filedata_fn (f),
            "desc" : desc_fmt % f,
            "outcome" : outcome } for f in [ "globals", "locals" ] ]

