description = "check that replace2 works"

filedata = """
{$ 
    locals { data, out }
    data = "cat dog foo.edu elephant bar.com eagle turtle x.jam.baz.net";
    out = replace (data, "[a-z.]+\.(edu|net|com)", "$1:$0");
    print (out);
    print (" ");
    out = replace (data, "[a-z.]+\.(edu|net|com)", 
                   lambda (v) { return ("%{toupper(v[1])}:%{v[0]}"); });
    print (out);

$}"""

tmp = "cat dog EDU:foo.edu elephant COM:bar.com eagle turtle NET:x.jam.baz.net"
outcome = tmp.lower () + " " + tmp
