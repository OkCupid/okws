test = "test of unicode biotches!"

filedata = """
{$
   locals {
     chinese : "\\xe5\\xaf\\x9f\\xef\\xbc\\x9a\\xe6\\xb2\\xb3\\xe5\\x8c\\x97\\xe7\\x87\\x95\\xe9\\x83\\x8a\\xe6\\xa5\\xbc\\xe5\\xb8\\x82\\xe6\\x8e\\xa8\\xe5\\x87\\xba\\xe6\\x9c\\x80\\xe4\\xb8\\xa5\\xe6\\x88\\xbf\\xe8\\xb4\\xb7\\xe4\\xbb\\xa4",
     french : "Pour P\\u00e9kin, Paris est loin d'\\u00eatre une puissance \\u00e9gale",
      obj : [ chinese, french ]
   }
   print (chinese + "\n" + french + "\n");
   print (obj);
$}

""" 

outcome = "\xe5\xaf\x9f\xef\xbc\x9a\xe6\xb2\xb3\xe5\x8c\x97\xe7\x87\x95\xe9\x83\x8a\xe6\xa5\xbc\xe5\xb8\x82\xe6\x8e\xa8\xe5\x87\xba\xe6\x9c\x80\xe4\xb8\xa5\xe6\x88\xbf\xe8\xb4\xb7\xe4\xbb\xa4 "  + \
"Pour P\xc3\xa9kin, Paris est loin d'\xc3\xaatre une puissance \xc3\xa9gale " + \
"[\"\\u5bdf\\uff1a\\u6cb3\\u5317\\u71d5\\u90ca\\u697c\\u5e02\\u63a8\\u51fa\\u6700\\u4e25\\u623f\\u8d37\\u4ee4\", " + \
"\"Pour P\u00e9kin, Paris est loin d'\u00eatre une puissance \u00e9gale\"]"
