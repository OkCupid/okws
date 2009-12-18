desc = "lazily int conversion (28.py in OKWS 2.1)"

filedata="""
{$  
     locals { x : "0xfff" }
     print x, "\n"
     if (x > 0xff) { print "ok1 " }
     if (x < 0xffff) { print "ok2 "}
$}
"""

outcome = "0xfff ok1 ok2"
    
