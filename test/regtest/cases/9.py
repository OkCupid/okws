
filedata = [
'X--{$ load ("$[1]") $}${x}--Y\nLine2\n',
'\n\n\n{$ set { x : "abcdef" } $}\n\n\n\n' ]

desc = "test of the load call, to make sure it doesn't introduct white space"

outcome_exact = "X--abcdef--Y\nLine2\n"
