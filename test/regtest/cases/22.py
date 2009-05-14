
desc = "basic booleans"

filedata="""
{$ set { x : { val : true } } $}
${x.val}
${x.val == true}
${x.val != true}
${!x.val == true}
${!x.val}
"""

outcome="true true false false false"
