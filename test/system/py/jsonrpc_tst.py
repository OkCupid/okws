
import time
import sys
import json
import base64

sys.path += [ "../../../py" ]

import jsonrpc

cli = jsonrpc.Client (host = "127.0.0.1", port = 10808)

cli.connect ()
cli.set_fast_decode (True)

C = cli.fetch_constants ()

cli.set_prog (C.TST2_PROG, C.TST2_VERS)

print cli.call (C.TST2_NEGATE, True)

print "XX"

print cli.call (C.TST2_NULL, None)

serial = str (int (time.time ()) % 10000)

key = "test-key-" + str (int (time.time ()) % 10000)

date = { "date" : { "on" : True, 
                    "date" : { "mday" : 27, "mon" : 11, "year" : 1977 } },
         "time" : { "on" : False} }
data = { "d" : date, "i" :  - serial , "pk" : 4008, "d2" : date}
put_arg = { "key" : key,
            "data" : data }

print cli.call (C.TST2_PUT, put_arg)

print cli.call (C.TST2_GET, key)

foo = { "b" : True,
        "bar" : { "datum1" : [],
                  "datum2" : [ data ],
                  "data" : [ data, data, data ],
                  "odata" : base64.b64encode ("\x44\xe0\xa4") } }
        
print cli.call (C.TST2_FOO_REFLECT, foo)

print cli.call (C.TST2_SUM, [ i * 2**43 for i in range (0,20) ] )


try:
    print cli.call (4000, foo)
except jsonrpc.Error, e:
    print "Good! Got expected error: " + str (e)
