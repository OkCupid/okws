description = "test of a crasher having to do with parsing of JS sections"

filedata = """
{$
   if (true) {{
        <script src="http://foo.com/" language="JavaScript"></script>
    }}
$}
"""

outcome = '<script src="http://foo.com/" language="JavaScript"></script>'

