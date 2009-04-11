
import os
import time
import sys
import signal
import glob

##=======================================================================

class RegTestError (Exception):
    def __init__ (self, s):
        self._s = s
    def __str__ (self):
        return repr (self._s)

##=======================================================================

verbose = True

def msg (m):
    if verbose:
        for l in m.split ('\n'):
            print l


##=======================================================================

def strip_ext (f, ext):
    ln = len (ext)
    if (f[-ln:] == ext):
        f = f[0:-ln]
    return f 

##=======================================================================

class Const:

    config = "test/system/okws_config"
    exe = "okd/okld"
    test = [ Const.exe, "-f", Const.config ]
    port = 8081
    hostaddr = "127.0.0.1"

    def __init__ (self):
        pass

##=======================================================================

class TestCase:

    def __init__ (self, d):
        for k in d.keys ():
            setattr (self, "_" + k, d[k])

##=======================================================================

class TestCaseLoader:

    ##----------------------------------------

    def __init__ (self):
        pass

    ##----------------------------------------

    def load_dir (self, d):
        v = []
        files = glob.glob ("%s/*.py" % d)
        for f in files:
            v += load_file (f)

    ##----------------------------------------

    def load_file (self, f):

        f = strip_ext (f, ".py")
        try:
            mod = __import__ (f)
        except ImportError, e:
            raise RegTestError, "failed to import test case: %s", f

        v = []
        n = 0

        try:
            for c in mod.cases:
                v["name"] = "%s.%d" (f, n)
                v += [ TestCase (c) ]
                n++;
        except AttributeError, e:
            pass



        return v

    ##----------------------------------------

    def load (self, inlist):
        out = []
        for f in inlist:
            if not os.exists (f):
                raise RegTestError, "file does not exist: %s" % f
            elif os.isdir (f):
                out += self.load_dir (f)
            elif os.isfile (f):
                out += self.load_file (f)
            else:
                raise RegTestError, "file does not exist: %s" % f

        return out

##=======================================================================

class OkwsServerInstance:

    def __init__ (self, cnst):
        self._cnst = cnst

    def run (self):
        pid = os.fork ()

        if pid == 0:
            cmd = self._cnst.test
            # child
            log = "okws.log"
            f = open (log, "w")
            msg ("[%d] running OKWS (to log '%s')" % (os.getpid (), log))
            for i in [ 1, 2 ]:
                os.close (i)
                os.dup (f.fileno ())
            os.execv (cmd[0], cmd)
            os.exit (0)

        else: 
            self._pid = pid

    def kill (self):
        pid = self._pid
        os.kill (pid, signal.SIGTERM)
        rc = os.wait4 (pid, 0)[1]
        msg ("[%d] OKWS exit with rc=%d" % (pid, rc))

##=======================================================================

class RegTester:
    
    def __init__ (self, cnst, files):
        self._cnst = cnst
        self._okws = OkwsServerInstance (cnst)
        self._files = files

    def run (self):
        self._okws.run ()
        time.sleep (3)
        self._okws.kill ()

##=======================================================================

def main (argv):
    c = Const ()
    r = RegTester (c, files)
    r.run ()

##=======================================================================

main (sys.argv)
