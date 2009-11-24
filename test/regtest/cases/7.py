
desc = "test scoping v1"

filedata = \
"""
{$
    def h() {
       return tmp;
    }

    def g() { 
       locals { tmp : 5 }
       return tmp + h ();
    }

    def f() {
      locals { tmp : 10 }
      return tmp + g ();
    }

    tmp = 3;
    print (f(), "\n");

    globals { tmp : 1 }
    print (f(), "\n");
$}
"""

outcome = "18 16"

  
