import json

desc = "test of JSON non-ascii characters (%s)"

chinese = "\xe5\xaf\x9f\xef\xbc\x9a\xe6\xb2\xb3\xe5\x8c\x97\xe7\x87\x95\xe9\x83\x8a\xe6\xa5\xbc\xe5\xb8\x82\xe6\x8e\xa8\xe5\x87\xba\xe6\x9c\x80\xe4\xb8\xa5\xe6\x88\xbf\xe8\xb4\xb7\xe4\xbb\xa4" 

french = "Pour P\xc3\xa9kin, Paris est loin d'\xc3\xaatre une puissance \xc3\xa9gale"

filedata = """
{$
   locals {
     chinese : %(chinese)s ,
     french :  %(french)s, 
     obj : [ chinese, french ]
   }
   print (chinese + "\\n" + french + "\\n");
   print (obj);
   print ("\\n" + json_escape (chinese) + "\\n");
   print (json_escape (french) + "\\n");
    
$} 

""" % { "chinese" : repr (chinese),
        "french" : json.dumps (french) }

outcome_fmt = """
%(chinese)s
%(french)s
[%(chinese_e)s, %(french_e)s]
%(chinese_e)s
%(french_e)s
"""

outcome_unicode = outcome_fmt % { "chinese" : chinese,
                                  "french" : french,
                                  "chinese_e" : json.dumps (chinese),
                                  "french_e" : json.dumps (french) }

outcome_utf8 = outcome_fmt % { "chinese" : chinese,
                               "french" : french,
                               "chinese_e" : '"' + chinese + '"',
                               "french_e" : '"' + french + '"' }

cases = [ { "filedata" : filedata,
            "outcome" : outcome_unicode,
            "desc" : desc % "unicode" },
          { "filedata" : filedata,
            "outcome" : outcome_utf8,
            "desc" : desc % "utf8",
            "utf8json" : True } ]
