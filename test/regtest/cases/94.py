description = "floating point ops"

filedata = """
{$
  locals { f : 0.44, g : 0.22 }
  locals { d : f - g }
  locals { q : .66 / d,
           p : 0/0.5 }
  print (d, " ", q, " ", p)
$}"""

outcome = "0.22 3 0"
