desc = "test multiplication (20.py in OKWS 2.1)"

filedata = """
{$ if (-8 * 9 == 1 - 73) { print "ok" } 
   locals { x : 31 } $}
${4*3}
${-4*5}
${"foo"*3}
${x * 20}
${5+3*4}
${1*2*3*4*5*6}
{$ if (5*(4+43) == 5*4 + 5*43) { print "ok" } $}
"""

outcome= """
ok
12
-20
foofoofoo
620
17
720
ok
"""


