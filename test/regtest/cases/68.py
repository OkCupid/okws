##-----------------------------------------------------------------------

desc = "test configuration file inclusion, nested + sequential"

##-----------------------------------------------------------------------

filedata= [ 
"""
{$ 
   globals { switch_on : false }
   include ("$[1]");
   if (switch_on) {
       globals { 
            bar : 20,
            jam : "hello"
       }
       universals {
            jazz : "all"
       }
       switch_on = false;
   }
   universals { "pickle" : "half-sour" }
$}
""", 
"""
{$
   switch_on = true;
$}
""",
"""
{$
   universals { blah : "blahblah" }
   if (jam) {
       globals { "whoop" : 100 }
   }
   if (!jazz) {
      whoop = 200;
   }
   locals { "jazz" : false }
   if (!jazz) {
      pickle = "dill";
   }
$}
""" ]

##-----------------------------------------------------------------------

display = [ ("switch_on", "false"),
            ("bar", 20),
            ("jam", "hello"),
            ("jazz", "all"),
            ("blah" , "blahblah"),
            ("whoop", 100 ),
            ("pickle", "dill") ]

##-----------------------------------------------------------------------

script_path = "configtest?global=1&fn=$[0]&fn=$[2]&display=" + \
    ','.join ([p[0] for p in display])

##-----------------------------------------------------------------------

outcome= "\n".join (["%s -> %s" % p for p in display ])

##-----------------------------------------------------------------------
