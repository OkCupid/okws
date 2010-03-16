description = "make sure that error report works for for()"

filedata = """
{$
   locals { foo : "blah", bar : { a : 1 }, baz : 10 }
   for (i, foo) { print(i); }
   for (i, bar) { print(i); }
   for (i, baz) { print(i); }
   for (i, bazz) { print(i); }
$}
"""

viserr = True

outcome = "blah" * 4
