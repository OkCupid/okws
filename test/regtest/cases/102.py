description = "test a syntax error crasher"

no_error_page = True

filedata = """
{$
    for (x) {}

$}"""

outcome_empty =  True
