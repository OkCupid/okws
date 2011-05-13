description = "check the bitwise_and filter"

filedata = """
{$ 
   print (bitwise_and (1023,511,255,127,63,31,15));
$}"""

outcome = "15"
