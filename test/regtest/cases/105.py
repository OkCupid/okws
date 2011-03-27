description = "check that the exit statement works"

inner = """
Fee
Fi
Fo
{$ exit; $}
Fum
{%
   for (i, range (0, 200)) {{ stuff }}
   print ("boo");
%}
Endnote
"""

outer = """
Before
{% include ("$[1]") %}
After
"""

inner_res = "Fee Fi Fo"

cases = [

    { "filedata" : inner,
      "desc" : description + " 1",
      "outcome" : inner_res
      },

    { "filedata" : [ outer, inner ],
      "desc" : description + " 2",
      "outcome" : "Before " + inner_res + " After"
      }
      
]
