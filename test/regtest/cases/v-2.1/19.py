
desc = "infinite recursion observed in the while; p2/p3 compatibility"

filedata=[
"""
{$ set { badboy : "max",
         key    : "k1" } $}
{$ switch (key), (k1, "$[1]", { badboy : "${badboy}" } ) $}
""",
"""
%{badboy}
"""
]

outcome = "max"



