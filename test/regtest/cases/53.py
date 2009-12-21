filedata = [
'X--{$ load ("$[1]") $}${x}--Y\nLine2\n',
'\n\n\n{$ globals { x : "abcdef" } $}\n\n\n\n' ]

desc = "test of the load call, to make sure it doesn't introduct white space" + \
	"(9.py from OKWS/2.1)"

outcome_exact = "X--abcdef--Y\nLine2\n"
