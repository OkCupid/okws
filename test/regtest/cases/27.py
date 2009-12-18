desc="basic test of assignments (case 1) (29.py from OKWS 2.1)"

filedata="""
{$
    locals { x : 10, z : 4 }
    x = y = 20;
    v1 = [x,y,z]
    locals { v2 : [ x, y, z ] }
    v2[1] = v2[2];

    print x , " " , v1 , " " , v2
$}
"""

outcome = "20 [20, 20, 4] [20, 4, 4]"

