
desc = "see if integers can act properly as ints while being lazily converted"

filedata="""
{$  
     setl { x : "0xfff" }
     print x, "\n"
     if (x > 0xff) { print "ok1 " }
     if (x < 0xffff) { print "ok2 "}
$}
"""

outcome = "0xfff ok1 ok2"
    
