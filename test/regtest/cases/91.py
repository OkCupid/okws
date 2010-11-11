test = "a crasher that rick found; another that eli found"

filedata = """
{$
   locals { d : { b : [ lambda() { return 1; } , decorate ] } }
   print (d);
$}
"""

outcome = '{"b" : ["<anonymous-lambda @ /regtest-scratch/cases_91.html:1>", ' \
    + '"<compiled function> rfn3:decorate"]}' 
