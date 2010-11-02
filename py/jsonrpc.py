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
        call = 0
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
        packet = self._socket.recv (packlen)

        (xid_in, reply) = struct.unpack (">LL", packet[0:8])
        if xid_in != xid:
            raise Error, "bad xid (%d != %d)" % (xid, xid_in)
        if reply != 1:
            raise Error, "expected reply bit, got %d" % reply
        l = len (packet)
        start = 24

        if l < start:
            raise Error, "packet too small, expected at least 24b, got %d" % l

        if l == start:
            res = None
        else:
            data = packet[start:]

            # strip off any trailing null bytes
            end = data.find ("\x00")
            if end > 0:
                data = data[:end]

            # eval json to python; might want to make this more robsuto
            res = json.loads (data)

        return res

    #-----------------------------------------

    def call (self, proc, arg, prog = -1, vers = -1):
       
        (packet, xid) = self.make_packet (proc, arg, prog, vers)
        self._socket.send (packet)
        return self.receive_packet (xid)
