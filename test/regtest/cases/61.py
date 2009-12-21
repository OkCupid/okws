
desc = "test the enumerate() library function"

filedata = """
{$
   locals { v :  [ "a", "b", "c", "d" ] };
   for (p, enumerate (v)) {
      print (p[0], " ", p[1], "\n");
   }
$}"""

outcome = "0 a 1 b 2 c 3 d"
