
d = "test addition on vectors!"

f = """
{$
    setle { v1 : [ 1, 2, 3 ] }
    setle { v2 : [ "dog", "cat", [ "turkey" ] ] }
    setle { v3 : v1 + v2 + [ "pig" ] }

    v3[0] = 10;
    print (v3, " " , v1, " ", (v1 + v2)[v1[2]]);

$}"""

o = '[10, 2, 3, "dog", "cat", ["turkey"], "pig"] ' + '[1, 2, 3]' + " dog"

case = { "filedata" : f,
         "desc" : d,
         "outcome": o }

# make sure it works multiple time and doesn't keep updating v
cases = [ case for i in range (0,3) ]

