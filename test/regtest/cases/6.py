#
# simple reg test for testing simple mathematical ops
#

filedata = """

{$ if (!0)               { print "0 " }
   if (1 < 20)           { print "1 " }
   if (-5 == 1 - 6)      { print "2 " }
   if (!(1 >= 20))       { print "3 " }
   if (20 + 30 > 40 + 3) { print "4 " }
   if (30 != -30)        { print "5 " }
   if (1 + (2 - (3 + (4 - 5))) == 1) { print "6 " }
   if (1 < 2 && 3 < 4)   { print "7 " }
   if (1 > 2 || 3 < 4)   { print "8 " }
   if (10 > (4 + 5) && !(30 + 1 + 1 > 100)) { print "9 " }
$}   
"""

outcome = " ".join (["%d" % i for i in range (0, 10) ]) + "\n"

desc = "simple arithmetic operations"
   
  
       

