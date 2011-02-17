
description = "test the list() function"

filedata = """
{$
    locals { s : "foobar",
             i : 100,
             d : { a : 10, "b" : 20 },
             l : [ 'a', 'b', 'c' ] }
    print (join (" ", [ list (s), list (i), list (d), list (l) ]));
$}
"""

outcome = " ". join ([ '["f", "o", "o", "b", "a", "r"]',
                       '["1", "0", "0"]',
                       '[["b", 20], ["a", 10]]',
                       '["a", "b", "c"]' ])
            
