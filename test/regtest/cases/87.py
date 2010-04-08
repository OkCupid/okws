description = "testing floats"

filedata = """{%
    locals { v : 10.48 + 10.43 }
    locals { x : v * 3.44 }
    print (format_float ("%0.8g", x), " ", format_float ("%0.3g", x), " ", x);
%}"""

outcome = "71.9304 71.9 71.9304"
