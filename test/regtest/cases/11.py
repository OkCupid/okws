
desc = "test of load/globals and including a variable (16.py in OKWS 2.1)"

filedata = [
"""
{$ load ("$[1]") $}
{$ include (fn) $}
""",
"""{$ globals { fn : "$[2]" } $}""",
"found me" ]

outcome = "found me"

