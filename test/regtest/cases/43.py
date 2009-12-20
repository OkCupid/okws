desc = "test json2pub"

filedata ="""
{$
   locals { s : "{ 'a' : [0 ,1 , 2, { 'b': 4, 'cow' : [ 0, 'boy' ] } ] }" }
   locals { obj : json2pub (s) }
   print (obj.a[3].cow[1]);
$}"""

outcome = "boy"

