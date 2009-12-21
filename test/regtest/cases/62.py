
desc = "test that define'd lambda can include other files..."

filedata = [
"""
{$
    def include_foo (x) {
        include (x);
        return true;
    }
    include_foo ("$[1]");
    include_foo ("$[2]");
    if (include_foo ("$[3]") && include_foo ("$[4]")) { print "ok"; }
$}

""",
"one ",
"two ",
"three ",
"four "
]

outcome = "one two three four ok"
