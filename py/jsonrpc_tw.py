"""
A twisted implemention of JSON-RPC for better conncurrency.
"""

import jsonrpc
from twisted.application import internet, service
from twisted.internet import defer, protocol, reactor
from twisted.protocols import basic
from twisted.python import components
from twisted.internet.defer import inlineCallbacks, Deferred
import sys

##-----------------------------------------------------------------------

def sleep (secs):
    d = Deferred()
    reactor.callLater(secs, d.callback, None)
    return d

##-----------------------------------------------------------------------

class AxprtConn (protocol.Protocol):
    def __init__ (self):
        pass
    def connectionMade (self):
        self.factory.associate (self)
        self.transport.write ("yo pimp daddy!")
    def dataReceived (self, data):
        print "got data: " + data
    def connectionLost (self, reason):
        pass
    
##-----------------------------------------------------------------------

def info (s):
        print "INFO: " + s

##-----------------------------------------------------------------------

def warn (s):
    print "WARN: " + s
 
##-----------------------------------------------------------------------

class Axprt (protocol.ClientFactory):

    protocol = AxprtConn

    ##------------------------------

    def __str__ (self):
        return "%s:%d" % (self._host, self._port)

    ##------------------------------

    def info (self, s):
        if self._verbose:
            info (str (self) + ": " + s)

    ##------------------------------

    def warn (self, s):
        warn (str (self) + ": " + s)

    ##------------------------------

    def __init__ (self, host, port, timeout = 2, verbose = False):
        self._host = host
        self._port = port
        self._timeout = timeout
        self._verbose = verbose
        self._first_cb = None
        self._conn = None

    ##------------------------------

    def clientConnectionFailed(self, connector, reason):
        self.warn ("connection failed")
        self._retry ()
    
    ##------------------------------

    def clientConnectionLost(self, connector, reason):
        self.warn ("connection closed")
        self._retry ()

    ##------------------------------

    def _launch (self):
        self.info ("launch")
        reactor.connectTCP(self._host, self._port, self)
        
    ##------------------------------

    @inlineCallbacks
    def _retry (self):
        if not self._first (False):
            self.info ("sleep %d" % self._timeout)
            yield sleep (self._timeout)
            self._launch ()

    ##------------------------------

    def _first (self, rc):
        ret = False
        if self._first_cb:
            c = self._first_cb
            self._first_cb = None
            c.callback (rc)
            ret = True
        return ret

    ##------------------------------

    def associate (self, x):
        self._conn = x
        self._first (True)

    ##------------------------------

    def connect (self, first_retry = True):
        """Connect an Axprt to the given host and port. Return a deferred
        that's fired after the first connection happens.  If the first
        connection fails, error out."""
        cb = Deferred ()
        if first_retry:
            self._first_cb = cb
        else:
            cb.callback (True)
        self._launch ()
        return cb

##-----------------------------------------------------------------------

@inlineCallbacks
def do_test ():
    x = Axprt (host = "localhost", port = 8000, verbose = True)
    ok = yield x.connect ()
    if ok:
        print "connection happened!"
    else:
        print "failure!"
        reactor.stop ()

##-----------------------------------------------------------------------

def main ():
    do_test()
    reactor.run()

# this only runs if the module was *not* imported
if __name__ == '__main__':
    main()
