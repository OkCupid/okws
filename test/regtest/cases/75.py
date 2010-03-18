
description = "test reference passing via include's"

filedata = ["""
{$
   locals { target : {} }
   for (i, range (0,2)) {
       include ("$[1]", { out : target, tmp : [ 1, 2, { "c" : 3 } ] } );
       print (target.foo, " ");
       include ("$[2]", { jam : [1, 11, 10], bar : "booo" });
   }
$}""",
"""
{$
    print (tmp[2].c, " ");
    tmp[2].c = 40;
    print (tmp[2].c, " ");
    out.foo = 10;

$}""",
"""
{$
    print (jam[1], " ");
    jam[1] = 400;
    print (jam[1], " ");

$}""" ]

outcome = "3 40 10 11 400 " * 2


