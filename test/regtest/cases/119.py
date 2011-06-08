description = "test fork"

test_s = "test string 1 2 3 bar foo baz"

filedata = """
{$
    def delayed_print (s) {
       sleep(* 1 *);
       print (s);
       logwarn (s);
    }
    locals { s : "%(test_s)s" }
    fork (lambda () { delayed_print(* s *); });
$}
{$
    sleep(* 2 *);
$}
XYZ""" % { "test_s" : test_s }



outcome = " XYZ"
