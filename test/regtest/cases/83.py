description = "null dictionary references"

filedata = """{%
   locals { d : {} }
   print (d.nodice, d["dice-free"], d[blah]);
%}"""

outcome_empty = True
