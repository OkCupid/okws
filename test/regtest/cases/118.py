description = "utf8 stuff"

chinese = "\xe7\xac\xbc\xe5\xa4\x84\xe7\xbd\xae\xe6\xbc\x94\xe7\xbb\x83"

filedata = """
{$
    locals { s : "%(chinese)s",
             t : substr (s, 2, 2) }
    print (len (s), " ", len (t), " ", t );
$}""" % { "chinese" : chinese }
outcome = "5 2 " + chinese[6:12]
