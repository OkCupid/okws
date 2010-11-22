
import time
import sys
import json
import base64
from twisted.internet import reactor
from twisted.internet.defer import inlineCallbacks, Deferred, returnValue

sys.path += [ "../../../py" ]

import jsonrpc
from jsonrpc_tw import Axprt, Aclnt

@inlineCallbacks
def run():
    x = Axprt (host = "localhost", port = 10808, verbose = True)
    ok = yield x.connect (True)
    C = yield x.fetch_constants ()
    print dir (C)
    cli = Aclnt (x, C.TST2_PROG, C.TST2_VERS)
    
    serial = str (int (time.time ()) % 10000)
    
    key = "test-key-" + str (int (time.time ()) % 10000)
    
    date = { "date" : { "on" : True, 
                        "date" : { "mday" : 27, "mon" : 11, "year" : 1977 } },
             "time" : { "on" : False} }
    data = { "d" : date, "i" : serial , "pk" : 4008, "d2" : date}
    put_arg = { "key" : key,
                "data" : data }
    
    res = yield cli.call (C.TST2_PUT, put_arg)
    print "put: " + str (res)
    
    res = yield cli.call (C.TST2_GET, key)
    print "get: " + str (res)
    
    foo = { "b" : True,
            "bar" : { "datum1" : [],
                      "datum2" : [ data ],
                      "data" : [ data, data, data ],
                      "odata" : base64.b64encode ("\x44\xe0\xa4") } }
        
    res = yield cli.call (C.TST2_FOO_REFLECT, foo)
    print "reflect: " + str (res)

    try:
        x = yield cli.call (4000, foo)
    except jsonrpc.Error, e:
        print "Good! Got expected error: " + str (e)

    reactor.stop ()

def main ():
    run ()
    reactor.run ()

main()



