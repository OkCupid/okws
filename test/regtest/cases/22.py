
desc = "basic booleans"

filedata="""
{$ set { x : { val : true } } $}
${x.val}
${x.val == true}
${x.val != true}
${!x.val == true}
${!x.val}
"""

outcome="1 true false false false"
