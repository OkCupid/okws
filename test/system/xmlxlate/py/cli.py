

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
    
server_url = "http://0.0.0.0:8081/xlater"
server = xmlrpclib.Server (server_url)
server.system.setDebugLevel (50)

C = RpcConst ()

C.setall (server.xdr.constants ( [ "tstprot_xml"] ) )

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : 4000,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC3,
      "arg" : { "p1" : [], 
                "p2" : [ { "xx" : C.XXC, "z" : 33 } ]
                }
      } )

print (res)

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : 4000,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC2,
      "arg" : { "xx" : C.XXA,
                "a" : { "x" : 40,
                        "y" : "foobarbar"}}
      } )


print (res)

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : 4000,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC1,
      "arg" : { "x" : 40,
                "y" : "footimetime"}
      } )

print (res)

