description = "test index_of"

filedata = """
{$
    print ([ index_of ("boboBEARbobo", "BEAR"),
             index_of ("bocboBEARbobo", "bo", 2),
             index_of ("blah", "b", 1000),
             index_of ("poop", "doop") ] )
$}"""

outcome = repr ([ 4, 3, -1, -1 ])
