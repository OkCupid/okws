
desc = "test the split function"

filedata = """
{$
    locals { v : [ "dog", "cat", "", "bar" ] }
    locals { y  : join ("-", v) }
    locals { z  : split (r/-/ , y) }
    print z;
$}"""

outcome = '["dog", "cat", "", "bar"]'

