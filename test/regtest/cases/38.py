
desc = "test the map() library function"

filedata = """
{$
    locals { m : { a : 1, b: 2, c: 3, d : 4 },
            v1 : [ "d", "d", "b", "z"] ,
            v2 : [ "a" , "b" , [ "c", "d" , "e", [ "a", "b" ] ] ] ,
            v3 : [ { foo : "a", bar : [ "b", "c" ] }, "c" ] }

    print (map (m, v1));
    print " ";
    print (map (m, v2));
    print " ";
    print (map (m, v3));
$}"""

outcome = "[4, 4, 2, null] " + \
    "[1, 2, [3, 4, null, [1, 2]]] " + \
    '[{"foo" : 1, "bar" : [2, 3]}, 3]'

    
