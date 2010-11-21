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
import struct
import json
from jsonrpc import Error, InPacket, OutPacket


##-----------------------------------------------------------------------

def sleep (secs):
    d = Deferred()
    reactor.callLater(secs, d.callback, None)
    return d

##-----------------------------------------------------------------------

class Buffer:

    ##-----------------------------------------

    def __init__ (self):
        self._dat = []
        self._len = 0

    ##-----------------------------------------

    def buffer (self, x):
        self._dat += [ x ]
        self._nb += len (x)

    ##-----------------------------------------

    def getlong (self):
        ret = None
        raw = self.get (4)
        if raw:
            (ret,_) = struct.unpack (">L", raw)
        return ret

    ##-----------------------------------------

    def get (self, n):
        if self._nb < n:
            return None
        ov = []
        templen = 0
        while templen < n:
            b = self._dat[0]
            templen += len (b)
            ov += [ b ]
            self._dat = self._dat[1:]
        out = ''.join ()
        if len (out) > n:
            self._dat = [ out[n:] ] + self._dat
            out = out[:n]
        self._nb -= n
        return out

##-----------------------------------------------------------------------

class AxprtConn (protocol.Protocol):

    ##-----------------------------------------

    def __init__ (self):
        self._buffer = Buffer ()
        self._packlen = -1
        self._dispatch = {}

    ##-----------------------------------------

    def connectionMade (self):
        self.factory.associate_conn (self)
        self.transport.write ("yo pimp daddy!")

    ##-----------------------------------------

    def dataReceived (self, data):
        self._buffer.buffer (data)
        raw = self.packetize ()
        if raw :
            p = InPacket (raw)
            try:
                p.decode ()
                cb = self._dispatch.get (p.xid)
                if cb:
                    cb.callback (p)
            except Error, e:
                self.factory.packet_drop (e)

    ##-----------------------------------------

    def packetize (self):
        if self._packlen < 0:
            elen = self._buffer.getlong ()
            if elen is not None:
                self._packlen = elen & 0x7fffffff

        ret = None
        if self._packlen >= 0:
            ret = self._buffer.get (self._packlen)
            if ret:
                self._packlen = -1
        return ret
            
    ##-----------------------------------------

    def connectionLost (self, reason):
        callbacks = self._dispatch.values ()
        self._dispatch = {}
        for c in callbacks:
            c.callback (None)

    ##-----------------------------------------

    def send (self, packet):
        (raw, xid) = packet.encode ()
        self.transport.write (raw)
        return xid

    ##-----------------------------------------

    def recv (self, xid):
        d = Deferred ()
        self._dispatch[xid] = d
        return d
    
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

    def packet_drop (self, e):
        self.warn ("packet drop: %s" % e)

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

    def associate_conn (self, x):
        self._conn = x
        self._first (True)

    ##------------------------------

    def got_packet (self, p):
        """AxprtConn calls this whenever we get a fully formed packet.
        We, in turn, should call whicher client it belongs to 
        (assuming there are multiple!)"""
        pass

    ##------------------------------

    def send (self, packet):
        return self._conn.send (packet)

    ##------------------------------

    def recv (self, xid):
        return self._conn.recv (xid)

    ##------------------------------

    def connect (self, first_retry = False):
        """Connect an Axprt to the given host and port. Return a deferred.
        Pass a flag to determine if the connections should be retried after 
        the first fails.

        If yes, then the deferred is triggered immediate with True.
        If no, then the deferred is triggered when the first connection
        fails or succeeds, with the verdict."""

        cb = Deferred ()
        if not first_retry:
            self._first_cb = cb
        else:
            cb.callback (True)
        self._launch ()
        return cb

##-----------------------------------------------------------------------

class Aclnt:
    """Corresponds to an sfslite aclnt.  Takes an Axprt and a prog/vers
    pair, and then you can make calls to it."""

    def __init__ (self, xprt, prog, vers):
        self._axprt = xprt
        self._prog = prog
        self._vers = vers

    ##-----------------------------------------

    def make_out_packet (self, proc, args):
        jsa = json.dumps (args)
        p = OutPacket (prog = self._prog, vers = self._vers, proc = proc, 
                       args = dat)
        return p

    ##-----------------------------------------

    def process_in_packet (self, packet):
        if packet.stat != rpc_msg.SUCCESS:
            raise Error, "non-success returned: %s" % \
                rpc_msg.stat_to_str (packet.stat)
        res = None
        if len (packet.data) > 0:
            res = json.loads (packet.data)
        return res

    ##-----------------------------------------

    @inlineCallbacks
    def call (self, proc, args):
        outp = self.make_out_packet (prog, args)
        xid = self._axprt.send (outp)
        inp = yield self._axprt.recv (xid)
        return self.process_in_packet (inp)

##-----------------------------------------------------------------------

@inlineCallbacks
def do_test ():
    x = Axprt (host = "localhost", port = 8000, verbose = True)
    ok = yield x.connect (True)
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
