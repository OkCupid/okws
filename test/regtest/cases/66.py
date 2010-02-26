desc = "strange parsing crasher that eli found"

filedata = """
{$
    d = { 'foo\"bar': "baz" }
    print (d);
$}
"""

outcome = '{"foo\\"bar" : "baz"}'
