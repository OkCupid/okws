
desc = "test scoping v1"

filedata = \
"""
{$
    def h() {
       return tmp;
    }

    def g() { 
       locals { tmp : 4 }
       return tmp + h ();
    }

    def f() {
      locals { tmp : 16 }
      return tmp + g ();
    }

    tmp = 1;
    print (f(), "\n");

    globals { tmp : 2 }
    print (f(), "\n");
$}
"""

outcome = "21 22"

  
