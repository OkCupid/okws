
filedata = [ """
<!--# include ("$[1]") -->
""",
"""
<!--# set ( { "FILE" => "$[2]" } ) -->
<!--# switch (DUMMY), (1,{{}}), (,{{

  {$ setl { action : "jackson" } $}
  Some free text.
  {$ if (1 + 10 == 2) {{ shitters }}
     else {{ 
        {$ setl { pickle : "dill" } $}
        {$ include ("${FILE}") $} }} $}

}}) -->
""",
"""
<!--#include ("$[3]", { "juice" => "orange"} ) -->
""",
"""
${action}
${juice}
${pickle}
"""]

desc = "test inclusions"

outcome="Some free text. jackson orange dill"
