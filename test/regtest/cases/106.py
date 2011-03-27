description = "test of the raw(**) filter"

inc = "{$ blah blah ${x} blah blah {{ stuff {% boo %} bluff }} biff $}"

filedata = [
"""
{$ print (raw(* "$[1]" *)); $}
""", inc
]

outcome = inc.replace ("$", "%")
