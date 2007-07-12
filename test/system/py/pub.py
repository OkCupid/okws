
import xmlrpclib
import sys

srv = "0.0.0.0:8081"

if len (sys.argv) > 1:
	srv = sys.argv[1]


server_url = "http://%s/xmlrpc" % (srv)
server = xmlrpclib.Server (server_url)
server.system.setDebugLevel (40)

res = server.okws.pub ( { "filename" : "/test/newswitch.html",
                          "options" : ( "debug",
                                        "includeInfo",
                                        "visibleErrors",
                                        "verbose") ,
                          "variables" : { "KEY" : 2 } } )
print (res['data'])

res = server.okws.pub ( { "filename" : "/conf/test.conf",
						  "conf" : True } )
print (res['data'])


