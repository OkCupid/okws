
desc = "test the dictionary functions: items(), keys, values()"

filedata = """
{$
    locals { d : { "a" : 1, "b" : [ 1, 2, 3], "c" : "C" } }
    print (join (",", keys (d)), "\n");
    for (p, items (d)) {
      print (p[0], " -> ", p[1], "\n");
    }
    for (k, keys (d)) {
      print (d[k], " ");
    }
    print ("\n");
$}"""

outcome =  '\n'.join (["a,c,b", 
                       "a -> 1",
                       "c -> C", 
                       "b -> [1, 2, 3]",
                       "1 C [1, 2, 3]"])
