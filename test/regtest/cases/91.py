test = "a crasher that rick found"

filedata = """
{$
   locals { d : { b : [ lambda() { return 1; }  ] } }
   print (d);
$}
"""
outcome = '{"b" : [<anonymous-lambda @ /regtest-scratch/cases_91.html:1>]}'
