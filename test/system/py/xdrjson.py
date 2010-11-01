
import time
import sys
import json

sys.path += [ "../../../py" ]

import jsonrpc

cli = jsonrpc.Client (host = "127.0.0.1", port = 10808,
                      prog = 10808, vers = 1)

cli.connect ()

serial = str (int (time.time ()) % 10000)

key = "test-key-" + str (int (time.time ()) % 10000)

date = { "date" : { "on" : True, 
                    "date" : { "mday" : 27, "mon" : 11, "year" : 1977 } },
         "time" : { "on" : False} }
data = { "d" : date, "i" : serial , "pk" : 4008, "d2" : date}
put_arg = { "key" : key,
            "data" : data }

print cli.call (1, put_arg)

print cli.call (2, key)

print json.dumps (eval (cli.call (92177, None)), indent = 4)
