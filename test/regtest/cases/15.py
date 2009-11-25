
desc = "test division (24.py from OKWS 2.1)"

filedata = """
{$ if ( 12/5 == 89/40)     { print "ok1 " }  
   if (-100/-10 == 211/20) { print "ok2" } 
   locals { x : 31, y : 6 - 5 }
$}

${4/3}
${-4/3}
${x/3}
${5+9/3}
${720/6/5/4}
${300/y}
"""

outcome = """
ok1
ok2 
1
-1
10
8
6
300
"""
   
