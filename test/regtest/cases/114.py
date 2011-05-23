description = "match and search regex with match return"

target = "fooaabbaabcddzab"
crud = "z332aawerr"

filedata = """
{$
    locals { x : r/foo([ab]*)(c(d+))?zab/, res : null }
    print (match (x, "T", "%(target)s"));
    print (" ");
    print (search (x, "T", "%(crud)s%(target)s%(crud)s"));
    print (" ");
    res = match (x, "T", "%(crud)s%(target)s%(crud)s");
    print (res);
    if (!res) { print " boo!" }
$}""" % { "target" : target, "crud" : crud } 

out = " ".join (repr ( [target, "aabbaab", "cdd", "dd"] ) for i in range (2) )
outcome = out.replace ("'", '"') + " [] boo!"
