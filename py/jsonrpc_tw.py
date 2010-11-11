"""
A twisted implemention of JSON-RPC for better conncurrency.
"""

import jsonrpc
from twisted.application import internet, service
from twisted.internet import defer, protocol, reactor
from twisted.protocols import basic
from twisted.python import components

##-----------------------------------------------------------------------

class Axprt (protocol.Protocol):
    def connectionMade (self):
        pass
    def dataReceived (self, data):
        pass
    def connectoinLost (self, reason):
        pass
    
##-----------------------------------------------------------------------

class AxprtFactory (protocol.ClientFactory):

    protocol = Axprt

    def __init__ (self):
        pass
