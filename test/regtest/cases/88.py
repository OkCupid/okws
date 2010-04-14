description = "testing bad RXXs"

filedata = """
{$
   split ('*', "yo*mamma*yo");
   split ('\*', "yo*mamma*yo");
   print ("made it");
$}"""

outcome = "made it"

