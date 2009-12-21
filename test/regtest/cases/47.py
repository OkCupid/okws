
filedata = [
"""
{$ load ("$[1]") $}
{$ include (fn) $}
""",
"""{$ globals { fn : "$[2]" } $}""",
"found me" ]

outcome = "found me"

desc = "test a case that daniel says doesn't work"
