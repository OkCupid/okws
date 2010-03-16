description = "make sure that error report works for for()"

filedata = """
{$
   locals { foo : "blah", bar : { a : 1 }, baz : 10 }
   for (i, foo) { print(i); }   print (" ");
   for (i, bar) { print(i); }   print (" ");
   for (i, baz) { print(i); }   print (" ");
   for (i, bazz) { print(i); }  print (" ");
$}
"""

viserr = True

outcome = " ".join ([ """<font color="red">[okws-pub3[eval]: /regtest-scratch/cases_71.html:%d: for: second argument is not an iterable vector]</font>""" % i  \
                          for i in range (4, 8) ])
