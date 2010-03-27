
desc = "test that define'd lambda can include other files..."

filedata = [
"""
{$
    def include_foo (x) {
        include (x);
        return true;
    }
    def return_fn (x) {
        include (x);
        return include_foo;
    }

    include_foo (* "$[1]" *);
    include_foo (* "$[2]" *);
    if (include_foo (* "$[3]" *) && include_foo (* "$[4]" *)) { print "ok "; }
    return_fn (* "$[1]" *)(* "$[2]" *);
$}

""",
"one ",
"two ",
"three ",
"four "
]

outcome = "one two three four ok one two"
