
desc = "test empty regions"

filedata = """
{$
    set { v : [] }

    for (i, v) {} empty {}
    for (i, v) {{}} empty {}
    for (i, v) {} empty {{}}
    for (i, v) {{}} empty {{}}
    for (i, v) { ;; } empty { ;;;;;; }
$}
{$ /* nothing */ $}
{$ $}
{$ ;;;; $}
"""

outcome=" "

