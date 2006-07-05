
import xmlrpclib
import sys

server_url = "http://0.0.0.0:8081/xmlrpc"
server = xmlrpclib.Server (server_url)
res = server.okws.pub ( { "filename" : "/test/newswitch.html",
                          "options" : ( "debug",
                                        "includeInfo",
                                        "visibleErrors",
                                        "verbose") ,
                          "variables" : { "KEY" : 2 } } )
print (res['data'])
