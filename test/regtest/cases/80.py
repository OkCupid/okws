
description = "trickier cases for bind/unbind"

filedata = """
{$
    def foo(y) {
       locals { x : 20 }
       for (i, ["l", "g"]) {
          print (lookup ("x", i), " ");
       }
       bind ("x", y, "g");
    }

    globals { x : 10 }
    foo(30);
    print(x);

$}"""


outcome = "20 10 30"
