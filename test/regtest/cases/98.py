test = "test of the localtime() function"

import time

times = [ 0, 100000, int (time.time()) ]

filedata = """
{$
   for (i, %(times)s) {
       locals { v : localtime(i) }
       print ("${v[0]} ${v[1]} ${v[2]} ${v[3]} ${v[4]} ");
   }
$}
""" % { "times" : times }

# in publand, localtime(0) should give the time now
times[0] = int (time.time())

outcome_v = []

for i in times:
    lt = time.localtime (i)
    outcome_v += [ lt.tm_year, lt.tm_mon, lt.tm_mday, lt.tm_hour, lt.tm_min ]

outcome = " ".join ([ str (i) for i in outcome_v ])
