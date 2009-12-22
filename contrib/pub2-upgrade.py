import re
import sys

##=======================================================================

def find_body (s, start):
    end = start
    count = 0
    go = True
    while go:
        if end >= len (s):
            go = False
        elif s[end] == '{':
            count += 1
        elif s[end] == '}':
            count -= 1
        if count == 0:
            go = False
        end += 1

    if count != 0:
        end = -1

    return end

##=======================================================================

class Zone:
    def __init__ (self):
        pass
    def replace_for (self): 
        pass

class Html (Zone):
    def __init__ (self, s):
        self._data = s
    def __str__ (self):
        return self._data

class Pub (Zone):

    def __init__ (self, s):
        self._data = s

    def __str__ (self):
        return self._data

    def replace_for (self):
        print "PP %s" % self._data[0:20]

        bod = r'for \((?P<iter>[0-9a-zA-Z_]+), (?P<vec>.+)\) {'
        for_rxx = re.compile (bod, re.S)
        dat = self._data
        out = []
        clear = False
        ret = False

        while (len (dat)):
            m = for_rxx.search (dat)
            print "XXXVFF %s" % m
            if m:
                d = m.groupdict ()
                start = m.start ()
                print "YY %s" % dat[start:start+30]
                body_start = m.end () - 1
                body_end = find_body (dat, body_start)
                if body_end > 0:
                    body = dat[body_start:body_end]
                    ix = d["iter"] + "\.(iter|first|last|even|odd|count)"
                    iter_rxx = re.compile (ix)
                    print "X %s" % ix
                    if iter_rxx.search (body):
                        out += [ dat[0:start],
                                 "for (%(iter)s, decorate (%(vec)s)) " % d  , 
                                 body ]
                        ret = True
                    else:
                        out += [ dat[0:body_end] ]
                    dat = dat[body_end:]
                else:
                    clear = True
            else:
                clear = True
            if clear:
                out += [ dat ]
                dat = ""

        self._data = ''.join (out)

        return ret
            

##=======================================================================

class Input:

    def __init__ (self, fh):
        self._lines = fh.readlines ()
        self._data = ''.join (self._lines)

        self._rc = 0  # row cursor
        self._cc = 0  # column cursor

    def split_zones (self):
        dat = self._data
        curr = ""
        rest = ""
        out = []

        html_mode = True 
        html_x = re.compile (r'({%|}})')
        pub_x = re.compile (r'(%}|{{)')

        while (len (dat)):
            if html_mode:
                m = html_x.search (dat)
                if m:
                    start = m.start ()
                    frag = dat[0:start]
                    dat = dat[start:]
                    out += [ Html (frag) ]
                    html_mode = False
                else:
                    out += [ Html (dat) ]
                    dat = ""
            else:
                m = pub_x.search (dat)
                if m:
                    start = m.start ()
                    frag = dat[0:start]
                    dat = dat[start:]
                    out += [ Pub (frag) ]
                    html_mode = True 
                else:
                    out += [ Pub (dat) ]
                    dat = ""
        self._zones = out

    def replace (self, fn):
        ret = False
        for z in self._zones:
            if fn (z):
                ret = True
        return ret

    def __str__ (self):
        return ''.join ([str (z) for z in self._zones])

##=======================================================================

i = Input (sys.stdin)
i.split_zones ()
i.replace (lambda x: x.replace_for ())
print str (i)



