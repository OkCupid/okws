"""
   jsonrpc

   A class that communicates over SUN-RPC but does marshalling via
   JSON-encoding, rather than XDR.
"""

import random
import socket
import struct
try: import simplejson as json
except ImportError: import json
import time

##-----------------------------------------------------------------------

class RetryableError (Exception):
    def __init__ (self, value):
        self._err = value
    def __str__ (self):
        return repr (self._err)

##-----------------------------------------------------------------------

class Error (Exception):
    def __init__ (self, value):
        self._err = value
    def __str__ (self):
        return repr (self._err)

##-----------------------------------------------------------------------

class RpcConst :
    """ Holds constants fetched through RPC calls """

    def __init__ (self, lst = [], jsres= {}):

        if jsres:
            # there are 5 constant lists, but treat them each the same;
            # eventually, we might do something smarter here
            for const_list in jsres.values ():
                lst += [ [ p["name"], p["value"] ] for p in const_list ]
        if lst:
            self.setall (lst)
    
    def set (self, lst):
        setattr (self, lst[0], lst[1])

    def setall (self, lst):
        for p in lst:
            self.set (p)

##-----------------------------------------------------------------------

class rpc_msg:
    """Rpc constants that can be found in /usr/include/rpc/rpc_msg.h"""

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
            klass.stat_map[getattr (klass, i)] = i

    ##----------------------------------------

    @classmethod
    def stat_to_str (klass, i):
        klass.init ()
        ret = klass.stat_map.get (i)
        if not ret:
            ret = "UNKNOWN_ERROR: %s" % i
        return ret

##-----------------------------------------------------------------------

class OutPacket:

    def __init__ (self, prog, vers, proc, data):
        self.prog = prog
        self.vers = vers
        self.proc = proc
        self.data = data

    def encode (self):

        # add the HEADER longs as follows:
        self.xid = random.randint(0,0xffffffff)
        call = rpc_msg.CALL
        js = 3

        self.hdr = struct.pack (">" + "L" * 6, 
                                self.xid, rpc_msg.CALL, js, 
                                self.prog, self.vers, self.proc)

        # make an XDR "Auth" Field
        self.hdr += struct.pack ("x" * 4 * 4) 

        # put in the arg payload
        self.inner = self.hdr + self.data

        # pad to a multiple of 4 bytes
        plen = len (self.inner)
        rem = plen % 4
        if rem != 0:
            self.inner += struct.pack ("x" * (4 - rem))

        # prepend the packet length
        encoded_len = len (self.inner) | 0x80000000;
        self.raw = struct.pack (">L", encoded_len) + self.inner 

        return (self.raw, self.xid)

##-----------------------------------------------------------------------

class InPacket:
    """Incoming packets are subjected to this class, that will go ahead
    and decode them.  The result of a decoding is setting the member
    fields: data, stat, mtype and xid. A jsonrpc.Error is thrown in the
    case of a bad decoding.

    We're following the decoding spec set forth in RFC-1057, which
    you can find here:

        http://www.ietf.org/rfc/rfc1057.txt

    In particular, we have this Packet format:

        union reply_body switch (reply_stat stat) {
         case MSG_ACCEPTED:
            accepted_reply areply;
         case MSG_DENIED:
            rejected_reply rreply;
         } reply;

        struct accepted_reply {
            opaque_auth verf;
            union switch (accept_stat stat) {
            case SUCCESS:
               opaque results[0];
               /*
                * procedure-specific results start here
                */
             case PROG_MISMATCH:
                struct {
                   unsigned int low;
                   unsigned int high;
                } mismatch_info;
             default:
                /*
                 * Void.  Cases include PROG_UNAVAIL, PROC_UNAVAIL,
                 * and GARBAGE_ARGS.
                 */
                void;
             } reply_data;
         }

         struct opaque_auth {
            auth_flavor flavor;
            opaque body<400>;
         };

    """

    #----------------------------------------

    def __init__ (self, r):
        self._n = len (r)
        self._raw = r
        self._p = 0

        # publically accessible fields that will be set in the case
        # of a successful decoding (see comment above)
        self.stat = rpc_msg.SUCCESS
        self.dat = None
        self.mtype = rpc_msg.CALL
        self.xid = 0

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

    def decode_opaque_auth (self):
        flavor = self.getlong ()
        len = self.getlong ()
        self.getbytes (len)

    #----------------------------------------

    def decode_accepted_reply (self):
        self.decode_opaque_auth ()
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

class JsonWrap:

    @classmethod
    def alloc (klass, o):
        if type (o) == dict:
            return JsonWrapDict (o)
        elif type (o) == list:
            return JsonWrapList (o)
        else:
            return o

##-----------------------------------------------------------------------

class JsonWrapDict (dict):

    def __init__ (self, d):
        dict.__init__ (self, d)
        for (k,v) in d.items ():
            k = k.encode("utf8")
            v = JsonWrap.alloc (v)
            self[k] = v
            setattr (self, k, v)

##-----------------------------------------------------------------------

class JsonWrapList (list):

    def __init__ (self, o):
        list.__init__ (self, o)
        for (i,v) in enumerate (o):
            v = JsonWrap.alloc (v)
            self[i] = v

##-----------------------------------------------------------------------

class Client:

    constant_prog = 79921

    #-----------------------------------------

    def __init__ (self, fd = -1, prog = None, vers = None, 
                  host = None, port = -1, retry = 2):
        self._fd = fd
        self._prog = prog
        self._vers = vers
        self._host = host
        self._port = port
        self._socket = None
        self._retry = retry
        self._fast_decode = False
        self._fancy_objs = True

    #-----------------------------------------

    def decode_json (self, obj):

        if self._fast_decode:
            true = True
            false = False
            null = None
            res = eval (obj)
        else:
            res = json.loads (obj)

        if self._fancy_objs:
            res = JsonWrap.alloc (res)

        return res

    #-----------------------------------------

    def set_fast_decode (self, b):
        self._fast_decode = b

    #-----------------------------------------

    def set_fancy_objs (self, b):
        self._fancy_objs = b

    #-----------------------------------------
    
    
    def connect (self, host = None, port = -1):
        if host: self._host = host
        if port >= 0: self._port = port
        self._socket = socket.create_connection ((self._host, self._port))

    #-----------------------------------------

    def fetch_constants (self):
        res = self.call (proc = 0, arg = None, vers = 1, 
                         prog = self.constant_prog)

        ret = RpcConst (jsres = res)
        self._const = ret
        return ret

    #-----------------------------------------

    def set_prog (self, prog, vers):
        self._prog = prog
        self._vers = vers

    #-----------------------------------------

    def make_packet (self, proc, arg, prog, vers):

        if prog < 0: prog = self._prog
        if vers < 0: vers = self._vers
        jsa = json.dumps (arg)
        p = OutPacket (prog, vers, proc, jsa)
        (dat, xid) = p.encode ()
        return (dat, xid)

    #-----------------------------------------

    def receive_packet (self, xid):

        # get the packet len, and decode via bitwise AND
        packlen_raw = self._socket.recv (4)
        
        if len(packlen_raw) == 0:
            raise RetryableError, "EOF on socket"

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

        packet = InPacket (raw)
        packet.decode ()

        if packet.xid != xid:
            raise Error, "bad xid (%d != %d)" % (xid, packet.xid)
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
            res = self.decode_json (payload)

        return res

    #-----------------------------------------

    def call_once (self, proc, arg, prog = -1, vers = -1):
       
        (packet, xid) = self.make_packet (proc, arg, prog, vers)
        self._socket.send (packet)
        return self.receive_packet (xid)

    #-----------------------------------------

    def err (self, msg):
        prfx = "jsonrpc.Client: %s:%d: " % (self._host, self._port)
        print prfx + msg

    #-----------------------------------------

    def __str__ (self):
        return "%s:%d" % (self._host, self._port)

    #-----------------------------------------

    def call (self, proc, arg, prog = -1, vers = -1):

        go = True
        need_connect = False
        while go:
            eof = False
            try:
                if need_connect:
                    self.connect ()
                ret = self.call_once (proc, arg, prog, vers)
            except RetryableError, e:
                self.err (str (e))
                eof = True
            except socket.error, e:
                self.err (str (e))
                eof = True

            if eof and self._retry:
                w = self._retry
                self.err ("connection dropped; retrying in %ds" % w)
                time.sleep (w)
                need_connect = True
            else:
                go = False

        return ret
