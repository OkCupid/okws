

import xmlrpclib
import sys

class RpcConst:

    def __init__ (self):
        pass
    
    def set (self, lst):
        setattr (self, lst[0], lst[1])

    def setall (self, lst):
        for p in lst:
            self.set (p)

if len (sys.argv) == 2:
	host = sys.argv[1]
else:
	host = "0.0.0.0:8081"
    
server_url = "http://" + host + "/xlater"
server = xmlrpclib.Server (server_url)
#server.system.setDebugLevel (4)

C = RpcConst ()

C.setall (server.xdr.constants ( [ "tstprot"] ) )

for i in range(0,10000):

    ull = 2**61 * 7 + 44
    ll = 0 - (2**62  + 33)
    
    res = server.xdr.xlate (
        { "hostname" : "127.0.0.1",
          "port" : 4000,
          "program" : "tst_prog_1",
          "procno" : C.TST_RPC4,
          "arg" : { "x" : "ui4:" + str (8484848),
                    "y" : "i8:" + str (ll),
                    "z" : "ui8:"+ str (ull)
                    }
          } )

    ull_post = long(res['z'][4:])
    ll_post = long (res['y'][3:])

    if ull != ull_post:
        raise ValueError, "RPC problem translating u_int64_t; " + \
              "got %x, expected %x" % (ull_post, ull)
    
    if ll != ll_post:
        raise ValueError, "RPC problem translating int64_t; " + \
              "got %d, expected %d" % (ll_post, ll)
