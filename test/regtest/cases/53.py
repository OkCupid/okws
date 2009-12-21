fd = [
'X--{$ load ("$[1]") $}${x}--Y\nLine2\n',
'\n\n\n{$ globals { x : "abcdef" } $}\n\n\n\n' ]

desc = "test of the load call, make sure it doesn't introduce white space" + \
	"(9.py from OKWS/2.1) (%s)"

outcome_pattern = "X--abcdef--Y%(space)sLine2%(space)s"

cases = [ { "filedata" : fd,
            "outcome_exact" : outcome_pattern % { "space" : "\n"},
            "desc" : desc % "no-WSS",
            "nowss" : 1 },
          { "filedata" : fd,
            "outcome_exact" : outcome_pattern % { "space" : " "},
            "desc" : desc % "WSS" } ]
