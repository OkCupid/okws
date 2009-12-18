desc = "test empty regions (31.py in OKWS 2.1)"

filedata = """
{$
    locals { v : [] }

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

