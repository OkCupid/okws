description = "testing that a function working like a dict doesn't crash us"

filedata = """{$
   locals { x : match.foo }
$}"""

outcome_empty = True
