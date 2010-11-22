"""
A twisted implemention of JSON-RPC for better conncurrency.
"""

import jsonrpc
from twisted.application import internet, service
from twisted.internet import defer, protocol, reactor
from twisted.protocols import basic
from twisted.python import components
from twisted.internet.defer import inlineCallbacks, Deferred, returnValue
import sys
import struct
import json
from jsonrpc import Error, InPacket, OutPacket, rpc_msg, RpcConst


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
        self._nb = 0

    ##-----------------------------------------

    def buffer (self, x):
        self._dat += [ x ]
        self._nb += len (x)

    ##-----------------------------------------

    def getlong (self):
        ret = None
        raw = self.get (4)
        if raw:
            pair = struct.unpack (">L", raw)
            ret = pair[0]
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
        out = ''.join (ov)
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
                    del self._dispatch[p.xid]
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
        self._conn_cb = None
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

    @inlineCallbacks
    def fetch_constants (self):
        cli = Aclnt (axprt = self,  
                     prog = jsonrpc.Client.constant_prog,
                     vers = 1)
        res = yield cli.call (proc = 0, arg = None)
        returnValue (RpcConst (jsres = res))

    ##------------------------------

    def associate_conn (self, x):
        self._conn = x
        self._first (True)
        if self._conn_cb:
            c = self._conn_cb
            self._conn_cb = None
            c.callback (x)

    ##------------------------------

    @inlineCallbacks
    def get_conn (self):
        x = self._conn
        if not x:
            cb = self._conn_cb = Deferred ()
            x = yield cb
        returnValue (x)


    ##------------------------------

    @inlineCallbacks
    def send (self, packet):
        x = yield self.get_conn ()
        returnValue ((x, x.send (packet)))

    ##------------------------------

    def recv (self, conn, xid):
        return conn.recv (xid)

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

    def __init__ (self, axprt, prog, vers):
        self._axprt = axprt
        self._prog = prog
        self._vers = vers

    ##-----------------------------------------

    def make_out_packet (self, proc, args):
        jsa = json.dumps (args)
        p = OutPacket (prog = self._prog, vers = self._vers, proc = proc, 
                       data = jsa)
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
    def call (self, proc, arg):
        outp = self.make_out_packet (proc, arg)
        (conn, xid)  = yield self._axprt.send (outp)
        inp = yield self._axprt.recv (conn, xid)
        res = self.process_in_packet (inp)
        returnValue (res)

##-----------------------------------------------------------------------

