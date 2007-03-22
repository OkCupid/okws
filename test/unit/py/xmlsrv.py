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

port = 8089
s = xmlsrv.SimpleXMLRPCServer (("0.0.0.0", port))
print 'Server on port %d' % port
s.register_function (foo)
s.serve_forever ()

