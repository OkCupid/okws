

import xmlrpclib
import sys
import struct

port = 4000

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
server.system.setDebugLevel (50)

C = RpcConst ()

C.setall (server.xdr.constants ( [ "tstprot"] ) )

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC11,
      "arg" : { "h1" : "ui8:10000000000",
                "h2" :  "ui8:9999999992",
                "sub" : True }
      } )
print res

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC9,
      "arg" : { "id" : "ui8:1",
                "questions" : [ { "questionid" : 10, "data" : "ui4:20" } ] }
      } )

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC3,
      "arg" : { "p1" : [], 
                "p2" : [ { "xx" : C.XXC, "z" : 33 } ]
                }
      } )

print (res)

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC2,
      "arg" : { "xx" : C.XXA,
                "a" : { "x" : 40,
                        "y" : "foobarbar"}}
      } )


print (res)

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC1,
      "arg" : { "x" : 40,
                "y" : "footimetime"}
      } )

print (res)

ull = 2**61 * 7 + 44
ll = 0 - (2**62  + 33)

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC4,
      "arg" : { "x" : "ui4:" + str (8484848),
                "y" : "i8:" + str (ll),
                "z" : "ui8:"+ str (ull)
                }
      } )

print (res)

ull_post = long(res['z'][4:])
ll_post = long (res['y'][3:])

if ull != ull_post:
    raise ValueError, "RPC problem translating u_int64_t; " + \
          "got %x, expected %x" % (ull_post, ull)

if ll != ll_post:
    raise ValueError, "RPC problem translating int64_t; " + \
          "got %d, expected %d" % (ll_post, ll)


try:
    
    res = server.xdr.xlate (
        { "hostname" : "127.0.0.1",
          "port" : port,
          "program" : "tst_prog_1",
          "procno" : C.TST_RPC2,
          "arg" : { "xx" : C.XXA,
                    "a" : { "x" : "a",
                            "y" : "xxyyy44"}}
          } )

except xmlrpclib.Fault, f:
    print 'Good; caught fault: %s' % f


try:
    
    res = server.xdr.xlate (
        { "hostname" : "127.0.0.1",
          "port" : port,
          "program" : "tst_prog_1",
          "procno" : C.TST_RPC2,
          "arg" : { "xx" : C.XXA,
                    "a" : { "x" : 1000,
                            "y" : 5000 }}
          } )

except xmlrpclib.Fault, f:
    print 'Good; caught fault: %s' % f


for i in range(0,10):

    arg = { "x" : 10,
            "a" : [1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
            "b" : """here is a very long long long long long string and more
			and more stuff and then some more stuff and even then some more
			stuff. Lots more stuff and also more and more and more stuff stuff
			stuff. Just a few more characters to push over pkt boundary.
			XXYYZZZ. abcdefwerwerwerwerrrweer. 21321313123939393939""",
            "c" : "here is a shorter shorter shorter string",
            "opq" : xmlrpclib.Binary (str ([struct.pack ("B", 
                                                         (i*i*13 + i*3)%256) 
                                            for i in range (0,1000) ])) }

    for i in range (1,6):
        n = "this_is_a_very_long_name_%d" % i
        arg[n] = i
            
 
    res = server.xdr.xlate (
        { "hostname" : "127.0.0.1",
          "port" : port,
          "program" : "tst_prog_1",
          "procno" : C.TST_RPC8,
          "arg" : arg
          })

    if res != 6888:
        print "XXX Problem: bad base64-decoding!"


res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC10,
      "arg" : { "iters" : "ui4:1",
                "bb" : { "x" : "ui4:1000", "y" : "i8:100000", 
                         "z" : "ui8:300000000"} 
                }
      } )

print (res)

res = server.xdr.xlate (
    { "hostname" : "127.0.0.1",
      "port" : port,
      "program" : "tst_prog_1",
      "procno" : C.TST_RPC10,
      "arg" : { "iters" : "ui4:0",
                "bb" : { "x" : "ui4:1000", "y" : "i8:100000", 
                         "z" : "ui8:300000000"} 
                }
      } )

print (res)
