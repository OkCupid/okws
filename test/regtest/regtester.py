
import os
import time
import sys
import signal

##=======================================================================

verbose = True

def msg (m):
    if verbose:
        for l in m.split ('\n'):
            print l


##=======================================================================

class Const:

    test = [ "okd/okld", "-f", "test/system/okws_config" ]
    port = 8081
    hostaddr = "127.0.0.1"

    def __init__ (self):
        pass


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
    
    def __init__ (self, cnst):
        self._cnst = cnst
        self._okws = OkwsServerInstance (cnst)

    def run (self):
        self._okws.run ()
        time.sleep (3)
        self._okws.kill ()

##=======================================================================

def main (argv):
    c = Const ()
    r = RegTester (c)
    r.run ()

##=======================================================================

main (sys.argv)
