
test_str = "foobar '<>'@!@#$%^&*^ that's it"

desc = "test the url_escape() library function"

filedata = """
{$
    locals { in : "%(test_str)s" }
    locals { esc : in|url_escape }
    locals { orig : esc|url_unescape }
    print esc, "\\n";
    print orig;
$}""" % { "test_str" : test_str }

outcome = "foobar+%27%3c%3e%27%40%21%40%23%24%25%5e%26%2a%5e+that%27s+it" \
    + " " + test_str

    
