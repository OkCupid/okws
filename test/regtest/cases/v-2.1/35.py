
desc = "test of the join() library function"

filedata="""
{$
   setl { sep : ",  ",
          x   : "X",
          y   : "Y" }

   setl { v : [ 1, 2, 10, x, y, "foo" ] }

   print (join (sep, v))
$}
"""

outcome = "1, 2, 10, X, Y, foo"
