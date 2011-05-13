description = "splice!"

filedata = """
{$ 
   locals { v : range (10), w  }
   w = splice (v, 3, 5, "dog", "cat");
   print (w);
$}"""

outcome = '[0, 1, 2, "dog", "cat", 8, 9]'
