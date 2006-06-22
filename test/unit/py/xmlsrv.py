#!/usr/bin/env python
#
#

import SimpleXMLRPCServer as xmlsrv

def foo (a,b,c,d):
    print a
    print b
    print c
    print d
    return (4, "foo" , (1, 2, {"hi" : 40, "bye": (1,2) } ) )

s = xmlsrv.SimpleXMLRPCServer (("0.0.0.0", 8089))
s.register_function (foo)
s.serve_forever ()

