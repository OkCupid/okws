
desc = "test negative vector indices" 

filedata = """
{$
   set { v : [ 10, 11, 12, 13 ] }
   print (v[-1], " ", v[-3], " ");
   v[10] = 100;
   v[-2] = 102;
   print (v[-1], " ", v[6], " " , v[9], " ");
$}"""

outcome = "13 11 100  102"
