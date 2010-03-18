
description = "check wss in a variety of places"

filedata = ["""
{$ enable_wss (true); $}
<script language='foo'> james  jams  blahs
 <textarea>blah blah
{$ include ("$[1]") $}
after    this     should    be  

strippped


strippped!
{$ enable_wss (false) $}
and   no   more    stripping
""",
"""</textarea> more  and   more</script>"""
]

outcome_exact = \
"""  <script language='foo'> james  jams  blahs
 <textarea>blah blah
</textarea> more  and   more</script> after this should be strippped strippped! 
and   no   more    stripping
"""
