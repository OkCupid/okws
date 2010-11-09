description = "check for a parse error: unclose environment"

no_error_page = True

filedata = """
This is an {$ if (3 == 2 + 1) {{ unclosed environment }}
"""

outcome_empty = True
