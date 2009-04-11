import os
import time
import sys
import signal
import glob
import re
from subprocess import Popen, PIPE
import copy

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

    okws_config = "test/system/okws_config"
    pub_config = "test/system/pub_config"
    
    okld_exe = "okd/okld"
    pub_exe = "client/pub"
    scratch = "regtest-scratch"
    static = "static"
    
    okld_test = [ okld_exe, "-f", okws_config ]
    pub_run = [ pub_exe, "-f", pub_config ]

    port = 8081
    hostname = "127.0.0.1"

    #-----------------------------------------

    def __init__ (self):
        self._jail_dir = None
        pass

    #-----------------------------------------

    def pub_const (self, c):
        """Calls upon the pub v3 command-line client to lookup the value
        of the given configuration variable."""

        v = copy.copy (self.pub_run)
        v += [ "-p", c ]
        pipe = Popen(v, stdout=PIPE).communicate()
        rxx = re.compile ("([^:]+):\\s+(.*)")
        for l in pipe:
            if l is not None:
                m = rxx.match (l.strip ())
                if m and m.group (1) == c:
                    r = m.group (2)
                    return r
        return None

    #-----------------------------------------

    def jail_dir (self):
        """Lookup the pub jail dir by calling into the pub v3 command
        line client.  Cache the result after we're done."""

        if self._jail_dir is None:
            self._jail_dir = self.pub_const ("JailDir")
        return self._jail_dir

    #-----------------------------------------

    def scratch_dir (self):
        d = self._jail_dir
        if d is not None:
            d = d + "/" + self.scratch
        return d

    #-----------------------------------------

    def static_url (self):
        return self.url (self.static)

    #-----------------------------------------

    def service_url (self, s):
        return self.url (s)

    #-----------------------------------------

    def url (self, loc):
        return "http://%s:%d/%s" % ( self.hostname, self.port, loc)

    #-----------------------------------------

    def make_scratch_dir (self):
        d = self.scratch_dir ()
        if d is None:
            raise RegTestError, "cannot find Pub jail or scratch dir"
        if os.path.exists (d):
            if !os.path.isdir (d):
                raise RegTestError, "scratch dir (%s) is not a dir!" % d
        else:
            try:
                os.mkdir (d)
            except OSError, e:
                raise RegTestError, e
        return d

##=======================================================================

def char_subst (s, f, r):
    out = ""
    for c in s:
        if c == f: c = r
        out += c
    return out

##=======================================================================

class TestCase:

    def __init__ (self, cnst, d):
        self._filedata = None
        self._desc = None
        self._outcome = None
        self._outcome_exact = None
        self._outcome_rxx = None
        self._service = None
        self._htdoc = None
        self._const = const

        for k in d.keys ():
            setattr (self, "_" + k, d[k])

        if not self._outcome \
                and not self._outcome_exact \
                and not self._outcome_rxx:
            raise RegTestError, "bad test case: no expected outcome given"

        if not self._filedata and not self._htdoc and not self._service:
            raise RegTestError, "bad test case: no input file given"

    ##----------------------------------------

    def translate_data (self, in_data):
        """Input data from Python files uses {$, $} and ${, instead
        of the equivalent commands with '%'.  This is to prevent us
        from doing lots of escaping when making test cases. This
        function makes the appropriate translation back."""

        rxx = re.compile ("({$|$}|${)")
        v_in = rxx.split (in_data)
        v_out = []
        i = 0
        for e in v_in:
            if (i % 2 == 0):
                v_out += [ e ]
            else:
                v_out += [ char_subst (e, '$', '%') ]
        return ''.join (v_out)
        
    ##----------------------------------------
    
    def name (self):
        return char_subst (self._name, '/', '-')

    ##----------------------------------------

    def filepath (self):
        n = self.name ()
        d = self._const.scratch_dir ()
        return "%s/%s" % (d, n)

    ##----------------------------------------

    def file_url (self):

        n = None
        if self._filedata:
            n = self.name ()
        elif self._htdoc:
            n = self._htdoc
        
        if n:
            u = self._const.static_url ()
            r =  "%s/%s" % (u, n)
        elif self._service:
            r = self._const.service_url (self._service)
        else:
            raise RegTestError, "cannot fine appropriate URL for '%s'" % \
                self._name

        return r

    ##----------------------------------------

    def write_data (self):
        """Some test cases write data out to the htdocs directory, rather
        than requiring that the data exists as part of OKWS.  This function
        is in charge of writing that data if needs be."""
        
        self._const.make_scratch_dir ()
        out = self.filepath ()
        dat = self.translate_data (self._filedata)
        f = fopen (out, "w")
        f.write (dat)

    ##----------------------------------------

    def fetch (self):
        u = self.file_url ()
        return urllib.urlopen (u)
        

##=======================================================================

class TestCaseLoader:

    ##----------------------------------------

    def __init__ (self, c):
        self._const = c

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

        try:
            v = self.load_cases (mod) 
            if not v:
                v = [ self.load_single_case (mod) ]
        except RegTestError, e:
            myerr (f, e)
            raise RegTestError, "cannot load test case file: %s" % f

        return v
            
    ##----------------------------------------

    def load_case (self, mod):
        """If the file has a single case, then it must have 
        the required fields as global data fields."""

        d = {}
        rxx = re.compile ("__.*__")
        for n in dir (mod):
            if not rxx.match (n):
                d[n] = getattr (mod, n)

        return TestCase (self._const, d)

    ##----------------------------------------

    def load_cases (self, mod):
        """If the file has specified multiple cases, then we're
        looking for an array named 'cases' that has one test
        case per entry."""

        v = []
        n = 0
        try:
            for c in mod.cases:
                v["name"] = "%s.%d" (f, n)
                v += [ TestCase (self._const, c) ]
                n += 1
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
    r = RegTester (c, argv)
    print c.pub_const ('JailDir')

##=======================================================================

main (sys.argv)
