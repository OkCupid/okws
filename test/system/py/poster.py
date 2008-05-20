
import urllib
import httplib
import random 
import sys
import hashlib
import getopt

progname = sys.argv[0]

##=======================================================================

class TestError (Exception):
    def __init__ (self, m):
        self._msg = m
    def __str__ (self):
        return self._msg

##=======================================================================

def emsg (s, b = None, e = None):
    v = s.split ('\n')
    if b is None:
        b = "** "
    if e is None:
        e = " **"
    for l in v:
        sys.stderr.write ("%s%s%s\n" %( b, l, e));

##=======================================================================

class StringGenerator:

    def __init__ (self):

        ascii =  \
            range (ord('a'), ord ('z')) + \
            range (ord ('A'), ord ('Z')) + \
            range (ord ('0'), ord ('9'))

        self._nicechars = [ chr(x) for x in ascii ]  + list ("-_+")
        self._allchars = self._nicechars + \
            list ("!@#$%^&*()=|\\\"';:<,.>/? ")

    def gen (self, n, nice = True):
        if nice:
            l = self._nicechars
        else:
            l = self._allchars
            
        upper = len (l) - 1
        return "".join([ l[random.randint(0,upper)] for x in range (n) ])

sg = StringGenerator()

##=======================================================================

class DataObj:

    def __init__ (self, lens = None, k = None, v = None):
        self._key = k
        self._value = v
        if lens is not None:
            self.gen (lens[0], lens[1])
        
    def gen (self, keylen, datlen):
        self._key = sg.gen (keylen, True);
        self._value = sg.gen (datlen, False)
        
    def dump (self):
        print "key: %s" % self._key
        print "value: %s" % self._value

    def fingerprint (self):
        return (self._key, len (self._value), 
                hashlib.sha1(self._value).hexdigest ())

    def dump2 (self):
        print "%s: %d %s" % self.fingerprint ()

    def toPair (self):
        return (self._key, self._value)

##=======================================================================

class DataSet:

    def __init__ (self, lens):
        self._objs = [ DataObj (lens = l) for l in lens ]

    def dump (self):
        for o in self._objs:
            o.dump ()

    def dump2 (self):
        for o in self._objs:
            o.dump2 ()

    def toDict (self):
        return dict (self.toList ())

    def toList (self):
        return [ o.toPair () for o in self._objs ]

    def params (self):
        return urllib.urlencode (self.toDict ())

##=======================================================================

class Test:
    
    ##----------------------------------------

    def __init__(self):
        self._host = "127.0.0.1"
        self._port = 8081
        self._service = "/posttest"
        self._num_fields = 4
        self._key_inc = 2
        self._val_mul = 4
        self._num_fields = 8
        self._randseed = None

    ##----------------------------------------

    def usage (self):
        print \
"""usage: %s -h<host> -p<port> -s<service> -n<num-fields> 
              -k<key-inc> -v<val-mul>""" % progname
        sys.exit (1)

    ##----------------------------------------

    def set_int (self, n, v):
        try:
            i = int (v)
            setattr (self, "_" + n, i)
        except ValueError, e:
            emsg ("Expected an int for %s; got %d" % (n, v))
            raise TestError, "bad argument"

    ##----------------------------------------

    def readArgs (self, argv):

        short_opts = 'h:p:n:k:v:s:r:'
        long_opts =  [ "host=",
                       "port=",
                       "num-fields=",
                       "key-inc=",
                       "val-mul=",
                       "service=",
                       "randseed=" ]

        self._argv = argv
        
        try:
            opts, args = getopt.getopt (argv[1:], short_opts, long_opts)
        except getopt.GetoptError:
            self.usage ()

        for o,a in opts:
            if o in ("-h", "--host"):
                self._host = a
            elif o in ("-p", "--port"):
                self.set_int ("port", a);
            elif o in ("-n", "--num-fields"):
                self.set_int ("num_fields", a);
            elif o in ("-k", "--key-inc"):
                self.set_int ("key_inc", a)
            elif o in ("-v", "--val-mul"):
                self.set_int ("val_mul", a)
            elif o in ("-s", "--service"):
                self._service = a
            elif o in ('-r', '-randseed'):
                self.set_int ("randseed", a)
            else:
                self.usage ()

    ##----------------------------------------

    def init (self):
        self._key_len = self._key_inc
        self._val_len = self._val_mul

        if self._randseed is not None:
            random.seed (self._randseed)
            
        self.genData ()

    ##----------------------------------------
        
    def genData (self):

        lst = []
        for i in range (0, self._num_fields):
            lst += [ (self._key_len, self._val_len) ]
            self._key_len += self._key_inc
            self._val_len *= self._val_mul
        self._data = DataSet (lst)

    ##----------------------------------------

    def dump2 (self):
        self._data.dump2 ()
        
##-----------------------------------------------------------------------

def main(argv):
    
    tst = Test()
    tst.readArgs(argv)
    tst.init()
    tst.dump2()

    return 0

##-----------------------------------------------------------------------

try:
    rc = main(sys.argv)
    sys.exit (rc)
except TestError, e:
    emsg ("exitting uncleanly (%s)" % e, "++ ", " ++")
    sys.exit (2)

