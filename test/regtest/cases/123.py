desc = "testing uint does _not_ round the last few bits"

filedata = \
"""{$
    locals { u1 : 1844674407370955161 }
    print(u1);
    print(uint(u1));
$}"""

outcome = """18446744073709551611844674407370955161"""
