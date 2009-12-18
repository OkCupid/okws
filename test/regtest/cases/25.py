desc = "cast strings, arrays, dicts to booleans (26.py from OKWS 2.1)"


filedata = """

{%
locals {  s1 :  "", 
        s2 : "yo B", 
        a1 : [], 
        a2 : [ "" ], 
        d1 : {}, 
        d2 : { "a" : "foo" } }
%}
%{!s1}
%{!s2}
%{!a1}
%{!a2}
%{!d1}
%{!d2}
"""

outcome = """ true false true false true false """
