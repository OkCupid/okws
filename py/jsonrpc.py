"""
   jsonrpc

   A class that communicates over SUN-RPC but does marshalling via
   JSON-encoding, rather than XDR.
"""

import random
import socket
import struct
import json

##-----------------------------------------------------------------------

class Error (Exception):
    def __init__ (self, value):
        self._err = value
    def __str__ (self):
        return repr (self._err)

##-----------------------------------------------------------------------

class RpcConst :
    """ Holds constants fetched through RPC calls """
    def __init__ (self, lst = []):
        if list:
            self.setall (lst)
    
    def set (self, lst):
        setattr (self, lst[0], lst[1])

    def setall (self, lst):
        for p in lst:
            self.set (p)

##-----------------------------------------------------------------------

class rpc_msg:

    ##----------------------------------------

    CALL=0
    REPLY=1
    MSG_ACCEPTED=0
    MSG_DENIED=1
    SUCCESS=0
    PROG_UNAVAIL=1
    PROG_MISMATCH=2
    PROC_UNAVAIL=3
    GARBAGE_ARGS=4
    SYSTEM_ERR=5
    
    stat_map = {}

    ##----------------------------------------

    @classmethod
    def init (klass):
        if klass.stat_map: return
        for i in [ "SUCCESS", "PROG_UNAVAIL", "PROG_MISMATCH",
                   "PROC_UNAVAIL", "GARBAGE_ARGS", "SYSTEM_ERR"] :
            klass.stat_map[i] = getattr (klass, i)

    ##----------------------------------------

    @classmethod
    def stat_to_str (klass, i):
        klass.init ()
        ret = klass.stat_map.get (i)
        if not ret:
            ret = "UNKNOWN_ERROR"
        return ret


##-----------------------------------------------------------------------

class Packet:

    #----------------------------------------

    def __init__ (self, r):
        self._n = len (r)
        self._raw = r
        self._p = 0
        self.stat = rpc_msg.SUCCESS
        self.dat = None
        self.mtype = rpc_msg.CALL

    #----------------------------------------

    def getbytes (self, n):
        need = n + self._p
        if self._n < n:
            raise Error, "packet too small; got %d bytes, needed %\n" % \
                (self._n, n)
        ret = self._raw[self._p : self._p+n]
        self._p += n
        return ret

    #----------------------------------------

    def getlongs (self, n = 1):
        dat = self.getbytes (4 * n)
        ret = struct.unpack (">" + "L" * n, dat)
        return ret

    #----------------------------------------

    def getlong (self):
        (ret,) = self.getlongs (1)
        return ret

    #----------------------------------------
    
    def decode_call (self):
        raise Error, "got unexpected call; expected replies only"

    #----------------------------------------

    def decode_verf (self):
        flavor = self.getlong ()
        len = self.getlong ()
        self.getbytes (len)

    #----------------------------------------

    def decode_accepted_reply (self):
        self.decode_verf ()
        tmp = self.getlong ()
        if tmp != rpc_msg.SUCCESS:
            self.stat = tmp
        else:
            dat = self._raw[self._p:]
            end = dat.find ("\x00")
            if end > 0:
                dat = dat[:end]
            self.data = dat

    #----------------------------------------
    
    def decode_reply (self):
        self.stat = self.getlong ()
        if self.stat == rpc_msg.MSG_ACCEPTED:
            self.decode_accepted_reply ()

    #----------------------------------------

    def decode (self):
        (self.xid, self.mtype) = self.getlongs (2)
        if self.mtype == rpc_msg.REPLY:
            self.decode_reply ()
        else:
            self.decode_call ()

##-----------------------------------------------------------------------

class Client:

    constant_prog = 79921

    #-----------------------------------------

    def __init__ (self, fd = -1, prog = None, vers = None, 
                  host = None, port = -1):
        self._fd = fd
        self._prog = prog
        self._vers = vers
        self._host = host
        self._port = port
        self._socket = None

    #-----------------------------------------
    
    def connect (self, host = None, port = -1):
        if host: self._host = host
        if port >= 0: self._port = port
        self._socket = socket.create_connection ((self._host, self._port))

    #-----------------------------------------

    def fetch_constants (self):
        res = self.call (proc = 0, arg = None, vers = 1, 
                         prog = self.constant_prog)
        lst = []

        # there are 5 constant lists, but treat them each the same;
        # eventually, we might do something smarter here
        for const_list in res.values ():
            lst += [ [ p["name"], p["value"] ] for p in const_list ]

        ret = RpcConst (lst)
        return ret

    #-----------------------------------------

    def set_prog (self, prog, vers):
        self._prog = prog
        self._vers = vers

    #-----------------------------------------

    def make_packet (self, proc, arg, prog, vers):

        # add the HEADER longs as follows:
        xid = random.randint(0,0xffffffff)
        call = rpc_msg.CALL
        js = 3

        if prog < 0: prog = self._prog
        if vers < 0: vers = self._vers
    
        hdr = struct.pack (">" + "L" * 6, xid, call, js, 
                           prog, vers, proc)

        # make an XDR "Auth" Field
        hdr += struct.pack ("x" * 4 * 4) 

        # put in the arg payload
        packet_inner = hdr + json.dumps (arg)

        # pad to a multiple of 4 bytes
        plen = len (packet_inner)
        rem = plen % 4
        if rem != 0:
            packet_inner += struct.pack ("x" * (4 - rem))

        # prepend the packet length
        encoded_len = len (packet_inner) | 0x80000000;
        packet = struct.pack (">L", encoded_len) + packet_inner

        return (packet, xid)

    #-----------------------------------------

    def receive_packet (self, xid):

        # get the packet len, and decode via bitwise AND
        packlen_raw = self._socket.recv (4)
        (packlen,) = struct.unpack (">L", packlen_raw)
        packlen = packlen & 0x7fffffff

        frags = []
        recv_len = 0
        while recv_len < packlen:
            frag = self._socket.recv (packlen - recv_len)
            if len (frag):
                recv_len += len (frag)
                frags += [ frag ]
        raw = ''.join (frags)

        packet = Packet (raw)
        packet.decode ()

        if packet.xid != xid:
            raise Error, "bad xid (%d != %d)" % (xid, xid_in)
        if packet.mtype != rpc_msg.REPLY:
            raise Error, "expected reply bit, got %d" % reply
        if packet.stat != rpc_msg.SUCCESS:
            raise Error, "non-success returned: %s" % \
                rpc_msg.stat_to_str (packet.stat)

        payload = packet.data

        if len (payload) == 0:
            res = None
        else:
            # eval json to python; might want to make this more robsuto
            res = json.loads (payload)

        return res

    #-----------------------------------------

    def call (self, proc, arg, prog = -1, vers = -1):
       
        (packet, xid) = self.make_packet (proc, arg, prog, vers)
        self._socket.send (packet)
        return self.receive_packet (xid)
