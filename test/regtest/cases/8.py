
desc = "basic test of file inclusion + scoping"

caller = \
"""
{$
     tmp = 10;
     locals { tmp : 3 }
     include ("$[1]", { param :  tmp });
$}

"""

callee = \
"""
{$ 
     print (param + tmp);
$}
"""

filedata = [ caller, callee ]

outcome = "13"
