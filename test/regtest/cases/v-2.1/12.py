
filedata="""

<!--#setl ( { "a" = "A"      } )  -->
{$   setl   {  b  : "B${a}"  }     $}
<!--#setl ( { "c" = "C$${b}" } )  -->
{$   setl   {  d  : "D${c}"  }     $}
<!--#setl ( { "e" = "E$${d}" } )  -->

%{d}
%{e}
"""

outcome = "DCBA EDCBA"

desc = "test nesting of pub3 inside of pub1 and vice-versa"
 

