
d = "test addition on vectors!"

f = """
{$
    locals { v1 : [ 1, 2, 3 ] }
    locals { v2 : [ "dog", "cat", [ "turkey" ] ] }
    locals { v3 : v1 + v2 + [ "pig" ] }

    v3[0] = 10;
    print ("[" + join (",", [ v3, v1, "'" + (v1+v2)[v1[2]] + "'" ] ) + "]" )

$}"""

o = [ [10, 2, 3, "dog", "cat", ["turkey"], "pig"], [1, 2, 3], "dog"]

case = { "filedata" : f,
         "desc" : d,
         "outcome_data": o }

# make sure it works multiple time and doesn't keep updating v
cases = [ case for i in range (0,3) ]

