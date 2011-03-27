description = "test of the raw(**) filter"

filedata = [
"""
{$ print (raw(* "$[1]" *)); $}
""",
"""
{$ blah blah ${x} blah blah $}
"""
]

outcome = "{% blah blah %{x} blah blah %}"
