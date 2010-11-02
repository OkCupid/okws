
import time
import sys
import json

sys.path += [ "../../../py" ]

import jsonrpc

cli = jsonrpc.Client (host = "127.0.0.1", port = 10808)

cli.connect ()

C = cli.fetch_constants ()

cli.set_prog (C.TST2_PROG, C.TST2_VERS)

serial = str (int (time.time ()) % 10000)

key = "test-key-" + str (int (time.time ()) % 10000)

date = { "date" : { "on" : True, 
                    "date" : { "mday" : 27, "mon" : 11, "year" : 1977 } },
         "time" : { "on" : False} }
data = { "d" : date, "i" : serial , "pk" : 4008, "d2" : date}
put_arg = { "key" : key,
            "data" : data }

print cli.call (C.TST2_PUT, put_arg)

print cli.call (C.TST2_GET, key)

