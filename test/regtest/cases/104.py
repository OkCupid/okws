description = "check that replace2 works"

filedata = """
{$ 
    locals { data, out }
    data = "cat dog foo.edu elephant bar.com eagle turtle x.jam.baz.net";
    out = replace2 (data, "[a-z.]+\.(edu|net|com)", "$1:$0");
    print (out);

$}"""

outcome = "cat dog edu:foo.edu elephant com:bar.com eagle turtle net:x.jam.baz.net"
