desc = "test negative vector indices (33.py in OKWS 2.1)"  

filedata = """
{$
   locals { v : [ 10, 11, 12, 13 ] }
   print (v[-1], " ", v[-3], " ");
   v[10] = 100;
   v[-2] = 102;
   print (v[-1], " ", v[6], " " , v[9], " ", isnull (v[6]));
$}"""

outcome = "13 11 100 102 true"
