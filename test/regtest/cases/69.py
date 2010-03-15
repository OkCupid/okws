desc = "elif .. elif .. else test"

filedata = """
{$ 
   def test (x) {   
      if   (x == 0) {{ blah }}
      elif (x == 1) {{ sham }}
      elif (x == 2) {{ shim }}
      else          {{ jello }}
   }
   for (i, range (5)) {
      test (i);
   }
$}
"""
outcome = "blah sham shim jello jello"
                



