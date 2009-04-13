#
# regtest of pub3 filters
#

test_str = "AbCdE FghIJKlmNOpQr FfooXx CCCccattTTtT DDogg"

filedata ="""
{$ setl { x : "%s" } $}
{$ setl { lcx : x | tolower,
          ucx : x | tolower | toupper ,
          z = "     a-floater    " } $}

orig: ${x}
lowered: ${lcx}
lowered (encore): ${x|tolower}
lowered (encore II): ${"%s"|tolower}
lowered-then-uppered: ${ucx}

html filtration: ${"<bad> stuff & more <crap>"|html_escape}
json filtration: ${"\\n\\t\\\\"|json_escape}
substring: ${"abcdefgh"|substr(2,4)}
substring (encore): ${substr("abcdefgh",2,4)}
strip-test: X---${z|strip()}---Y
""" % (test_str, test_str)

desc = "a test for filters"

outcome = """
orig: %s
lowered: %s
lowered (encore): %s
lowered (encore II): %s
lowered-then-uppered: %s

html filtration: &lt;bad&gt; stuff &#38; more &lt;crap&gt;
json filtration: "\\n\\t\\\\"
substring: cdef
substring (encore): cdef
strip-test: X---a-floater---Y
""" % (test_str, test_str.lower (), test_str.lower (),  test_str.lower (),
       test_str.upper ())
