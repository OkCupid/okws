description = "match and search regex with match return"

target = "fooaabbaabcddzab"
crud = "z332aawerr"

filedata = """
{$
    locals { x : r/foo([ab]*)(c(d+))?zab/, res : null }
    print (match (x, "%(target)s", "T"));
    print (" ");
    print (search (x, "%(crud)s%(target)s%(crud)s", "T"));
    print (" ");
    res = match (x, "%(crud)s%(target)s%(crud)s", "T");
    print (res);
    if (!res) { print " boo!" }
$}""" % { "target" : target, "crud" : crud } 

out = " ".join (repr ( [target, "aabbaab", "cdd", "dd"] ) for i in range (2) )
outcome = out.replace ("'", '"') + " [] boo!"
