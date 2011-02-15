description = "check wss for heredocs"

filedata = ["""
{$ 
   enable_wss (true);
   locals { a : ={ include ("$[1]") } }
   enable_wss (false);
   locals { b : ={ include ("$[1]") } };
   print (a);
   print (b);
$}""",
"""a   b   
c   d
e """
]

outcome_exact = """ a b c d e a   b   
c   d
e """
