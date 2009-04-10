
import xmlrpclib
import sys

srv = "0.0.0.0:8081"

if len (sys.argv) > 1:
    srv = sys.argv[1]

server_url = "http://%s/xmlrpc" % (srv)
server = xmlrpclib.Server (server_url)
server.system.setDebugLevel (10)

#
# Duplicate the obejcts in forloop.T
#
args = { "row" : [ { "val" : i, "col": [ { "val" : j } for j in range (0,4) ] }
                   for i in range (0,3) ],
         "simple" : [ {"val" : 2*i + 1} for i in range (0,4) ],
         "push" : [ { "val" : "xx-%d-xx" % i } for i in range (0,5) ],
         "single" : [ { "val" : "The-One-True-God" } ],
         "double" : [ { "val" : "Mike" }, { "val" : "Ike"} ] }

res = server.okws.pub3 ( { "filename" : "/test/forloop2.html",
                           "options" : ( "debug",
                                         "includeInfo",
                                         "visibleErrors",
                                         "verbose") ,
                           "variables" : args })
print (res['data'])

res = server.okws.pub3 ( { "filename" : "/conf/test.conf",
                           "conf" : True } )
print (res['data'])


