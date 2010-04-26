description = "test the new stat_file function"

filedata = [
"""
{$
   locals { d : stat_file(* "$[1]" *) }
   print (d.hash);
$}""",
"""blah blah blah blah blah blah blah 3"""]

outcome = "1ff8efaa3d7bb83d8348cfc9bbdef05bb126376d"
