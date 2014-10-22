#
# Preliminary testing of regular expressions.
#

inputs = [ "aaabcccc", "abc", "aaacccc>", "abab", "aAaAbCcCcC" ]
d = [ { "val" : s } for s in inputs ]

regex = "r<a+b?c+\\>?>"

#
# for the last test case, add 0 so that the results gets coerced into
# an int.  sneaky, i know!
#
filedata = """
{$ locals { v   : %s,
            rxx : %s } 
   for (i, decorate (v)) {

   print (i.iter + ": ")

   if   (match (rxx, i.val))       { print 1 }
   elif (match (rxx, i.val, "i"))  { print 2 }
   else                            { print 0 }

   print "\\n"

} $}

{$ locals { x : "XXaabccc>YY" } $}
%d: ${search(rxx,x) + 0}

""" % (d, regex, len (inputs))

results = [ 1, 1, 1, 0, 2, 1 ]

outcome = '\n'.join (["%d: %d" % (i, results[i]) 
                     for i in range (0, len (results))])

desc = "regular expression matches"
