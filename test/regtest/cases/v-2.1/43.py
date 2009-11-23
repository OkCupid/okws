
desc = "test the split function"

filedata = """
{$
    setle { v : [ "dog", "cat", "", "bar" ] }
    setle { y  : join ("-", v) }
    setle { z  : split (r/-/, y) }
    print z;
$}"""

outcome = '["dog", "cat", "", "bar"]'

