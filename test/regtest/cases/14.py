desc = "basic booleans (22.py in OKWS 2.1)"

filedata="""
{$ locals { x : { val : true } } $}
${x.val}
${x.val == true}
${x.val != true}
${!x.val == true}
${!x.val}
"""

outcome="true true false false false"
