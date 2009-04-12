import imp
import time
import sys
import signal
import glob
import re
from subprocess import Popen, PIPE
import copy
import os
import urllib
import getopt

##=======================================================================

#
# need this mod to get imp.* working properly...
#
sys.path = [''] + sys.path 

progname = sys.argv[0]

##=======================================================================

class RegTestError (Exception):
    def __init__ (self, s):
        self._s = s
    def __str__ (self):
        return repr (self._s)

##=======================================================================

INFO = 1
RESULT = 2
ERROR = 3

report_level = INFO

def msg (m, level = INFO):
    if level >= report_level:
        for l in m.split ('\n'):
            print l

def myerr (f, e):
    print >>sys.stderr, "XX %s: %s" % (f, e)

##=======================================================================

def strip_ext (f, ext):
    ln = len (ext)
    if (f[-ln:] == ext):
        f = f[0:-ln]
    return f 

##=======================================================================

def usage (rc):
    print \
"""usage: %s [-q|-v] [-c<casedir>] [-e] <case1> <case2> ...

    Run this testing harness from the top build directory.
 
    Flags
      -q, --quiet  
          Only make output on error
          
      -v, --verbose
          Output debug information and so on

      -e, --exaplain 
          don't run test case, just explain what it is

      -c <casedir>, --casedir=<dir>
          Supply a casedir to shorten the name of test cases; those
          cases given without absolute paths with be considered relative
          to the casedir. Typically set this equal to the top
          directory in the source code tree that relates to regtesting.

""" % (progname)
    sys.exit (rc)



##=======================================================================

class Config:

    okws_config = "test/system/okws_config"
    pub_config = "test/system/pub_config"
    
    okld_exe = "okd/okld"
    pub_exe = "client/pub"
    scratch = "regtest-scratch"
    static = "static"
    failed_files_dir = "regtest-failures"
    
    okld_test = [ okld_exe, "-f", okws_config ]
    pub_run = [ pub_exe, "-f", pub_config ]

    port = 8081
    hostname = "127.0.0.1"

    #-----------------------------------------

    def __init__ (self):
        self._casedir = None
        self._explain_only = False
        self._jail_dir = None
        pass

    #-----------------------------------------

    def parseopts (self, argv):
        short_opts = "c:eqv"
        long_opts = [ 'casedir=',
                      'explain',
                      'quiet',
                      'verbose' ]

        try:
            opts,args = getopt.getopt (argv, short_opts, long_opts)
        except getopt.GetoptError:
            usage (-1)

        for o,a in opts:

            if False: pass

            elif o in ("-c", "--casedir"):
                self._casedir = a
            elif o in ("-e", "--explain"):
                self._explain_only = True
            elif o in ("-q", "--quiet"):
                report_level = ERROR
            elif o in ("-v", "--verbose"):
                report_level = INFO

        return args

    #----------------------------------------

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
        d = self.jail_dir ()
        if d is not None:
            d = d + "/" + self.scratch
        return d

    #-----------------------------------------

    def static_url (self):
        return self.url (self.static)

    #-----------------------------------------

    def scratch_url (self):
        return self.url ("%s/%s" % (self.static, self.scratch))

    #-----------------------------------------

    def service_url (self, s):
        return self.url (s)

    #-----------------------------------------

    def url (self, loc):
        return "http://%s:%d/%s" % ( self.hostname, self.port, loc)

    #-----------------------------------------

    def write_failure_file (self, nm, data):
        d = self.failed_files_dir
        self.mkdir (d, "failures")
        fn = "%s/%s-%s" % (self.failed_files_dir, nm, "response");
        try:
            f = open (fn, "w")
            f.write (data)
        except OSError, e:
            raise RegTestError, "failure file failure: %s" % e
        return fn

    #-----------------------------------------

    def make_scratch_dir (self):
        d = self.scratch_dir ()
        self.mkdir (d, "scratch")
        return d

    #-----------------------------------------

    def expand_test_case (self, n):
        if n[0] != '/' and self._casedir:
            return '%s/%s' % (self._casedir, n)
        else:
            return n

    #-----------------------------------------

    def mkdir (self, d, desc):

        if d is None:
            raise RegTestError, "cannot make empty '%s' dir" % desc
        if os.path.exists (d):
            if not os.path.isdir (d):
                raise RegTestError, "%s dir (%s) is not a dir!" % (desc, d)
        else:
            try:
                os.mkdir (d)
            except OSError, e:
                raise RegTestError, e

##=======================================================================

def char_subst (s, f, r):
    out = ""
    for c in s:
        if c == f: c = r
        out += c
    return out

##=======================================================================

def dig_to_char (n):
    if n < 26:
        return "%c" % (ord('a') + n)
    elif n < 52:
        return  "%c" % (ord('A') + n - 26)
    else:
        return ".%d" % n

##=======================================================================

class TestCase:

    def __init__ (self, config, d):
        self._filedata = None
        self._desc = None
        self._outcome = None
        self._outcome_exact = None
        self._outcome_rxx = None
        self._service = None
        self._htdoc = None
        self._config = config 
        self._scratch_file = None
        self._result = False

        for k in d.keys ():
            setattr (self, "_" + k, d[k])

        if not self._outcome \
                and not self._outcome_exact \
                and not self._outcome_rxx:
            raise RegTestError, "bad test case: no expected outcome given"

        if not self._filedata and not self._htdoc and not self._service:
            raise RegTestError, "bad test case: no input file given"

    ##----------------------------------------

    @classmethod
    def translate_data (self, in_data):
        """Input data from Python files uses {$, $} and ${, instead
        of the equivalent commands with '%'.  This is to prevent us
        from doing lots of escaping when making test cases. This
        function makes the appropriate translation back."""

        rxx = re.compile ("({\\$|\\$}|\\${)")
        v_in = rxx.split (in_data)
        v_out = []
        i = 0
        for e in v_in:
            if (i % 2 == 0):
                v_out += [ e ]
            else:
                v_out += [ char_subst (e, '$', '%') ]
            i += 1
        return ''.join (v_out)
        
    ##----------------------------------------
    
    def name (self):
        return char_subst (self._name, '/', '_')

    ##----------------------------------------

    def filepath (self):
        n = self.name ()
        d = self._config.scratch_dir ()
        return "%s/%s.html" % (d, n)

    ##----------------------------------------

    def file_url (self):

        n = None
        if self._filedata:
            n = self.name ()
        elif self._htdoc:
            n = self._htdoc
        
        if n:
            u = self._config.scratch_url ()
            r =  "%s/%s.html" % (u, n)
        elif self._service:
            r = self._config.service_url (self._service)
        else:
            raise RegTestError, "cannot fine appropriate URL for '%s'" % \
                self._name

        return r

    ##----------------------------------------

    def write_data (self):
        """Some test cases write data out to the htdocs directory, rather
        than requiring that the data exists as part of OKWS.  This function
        is in charge of writing that data if needs be."""

        if not self._filedata:
            return
        self._config.make_scratch_dir ()
        out = self.filepath ()
        dat = self.translate_data (self._filedata)
        f = open (out, "w")
        f.write (dat)
        self._scratch_file = out

    ##----------------------------------------

    def cleanup (self):
        if self._result and self._scratch_file:
            os.unlink (self._scratch_file)

    ##----------------------------------------

    def fetch (self):
        u = self.file_url ()
        print "url: " + u
        resp = ''.join (urllib.urlopen (u))
        return resp

    ##----------------------------------------

    def compare (self, txt):
        res = False
        if self._outcome_exact:
            res = (self._outcome_exact == txt)
        elif self._outcome_rxx:
            rxx = re.compile (self._outcome_rxx)
            res = rxx.match (txt)
        elif self._outcome:
            res = self._outcome.split () == txt.split ()
        else:
            raise RegTestError, "comparison failed; no outcome found"
        self._result = res
        return res
        
    ##----------------------------------------

    def report_success (self):
        msg ("%s .... ok" % self.name (), RESULT)

    ##----------------------------------------

    def report_failure (self, problem):
        txt = "%s .... FAILED!! (%s)" % (self.name (), problem)
        msg (txt, ERROR)

    ##----------------------------------------

    def run (self):
        self.write_data ()
        d = self.fetch ()
        ret = False
        if d:
            res = self.compare (d)
            if res:
                ret = True
                self.report_success ()
            else:
                f = self._config.write_failure_file (self.name (), d)
                self.report_failure ("data mismatch; got '%s'" % f)
        else:
            self.report_failure ("empty reply")
        return ret
                
    ##----------------------------------------

##=======================================================================

class TestCaseLoader:

    ##----------------------------------------

    def __init__ (self, c):
        self._config= c

    ##----------------------------------------

    def load_dir (self, name, full):
        v = []
        files = glob.glob ("%s/*.py" % full)
        for file in files:
            this_full = file
            d,f = os.path.split (file)
            this_name = "%s/%s" % (name, f)
            
            v += self.load_file (this_name, this_full)
        return v

    ##----------------------------------------

    def load_file (self, name, full):
        name = strip_ext (name, ".py")
        full = strip_ext (full, ".py")

        try:
            r = imp.find_module (full)
            mod = imp.load_module ('rtmod', *r)
        except ImportError, e:
            raise RegTestError, "failed to load test case: %s" % full

        try:
            v = self.load_cases (name, full, mod)
            if not v:
                v = [ self.load_single_case (name, full, mod) ]
        except RegTestError, e:
            myerr (full, e)
            raise RegTestError, "cannot load test case file: %s" % full

        return v
            
    ##----------------------------------------

    def load_single_case (self, name, full, mod):
        """If the file has a single case, then it must have 
        the required fields as global data fields."""

        d = {}
        rxx = re.compile ("__.*__")
        for n in dir (mod):
            if not rxx.match (n):
                d[n] = getattr (mod, n)
                d["name"] = name
                d["fullpath"] = full

        return TestCase (self._config, d)

    ##----------------------------------------

    def load_cases (self, name, full, mod):
        """If the file has specified multiple cases, then we're
        looking for an array named 'cases' that has one test
        case per entry."""

        v = []
        n = 0
        try:
            for c in mod.cases:
                c["name"] = "%s%c" % (name, dig_to_char (n))
                c["full"] = full
                v += [ TestCase (self._config, c) ]
                n += 1
        except AttributeError, e:
            pass

        return v

    ##----------------------------------------

    def load (self, inlist):
        out = []
        for f in inlist:
            full = self._config.expand_test_case (f)
            if not os.path.exists (full):
                raise RegTestError, "file does not exist: %s" % full
            elif os.path.isdir (full):
                out += self.load_dir (f, full)
            elif os.path.isfile (full):
                out += self.load_file (f, full)
            else:
                raise RegTestError, "file does not exist: %s" % full

        return out

##=======================================================================

class OkwsServerInstance:

    ##-----------------------------------------

    def __init__ (self, config):
        self._config = config  

    ##-----------------------------------------

    def run (self):
        pid = os.fork ()

        if pid == 0:
            cmd = self._config.okld_test
            # child
            log = "okws.log"
            f = open (log, "w")
            msg ("[%d] running OKWS (to log '%s')" % (os.getpid (), log), INFO)
            for i in [ 1, 2 ]:
                os.close (i)
                os.dup (f.fileno ())
            os.execv (cmd[0], cmd)
            os.exit (0)

        else: 
            self._pid = pid

    ##-----------------------------------------

    def kill (self):
        pid = self._pid
        os.kill (pid, signal.SIGTERM)
        rc = os.wait4 (pid, 0)[1]
        msg ("[%d] OKWS exit with rc=%d" % (pid, rc), INFO)

##=======================================================================

class RegTester:
    
    def __init__ (self, config):
        self._config = config 
        self._okws = OkwsServerInstance (config)
        self._loader = TestCaseLoader (config)

    ##-----------------------------------------

    def run_file (self, f):
        rc = True
        return rc

    ##-----------------------------------------

    def run (self, files):
        rc = True
        self._okws.run ()

        try:
            time.sleep (2)

            v = self._loader.load (files)
            for c in v:
                if not c.run ():
                    rc = False
        finally:
            # make sure we alway clean up after ourselves, that way
            # OKWS won't leave a stale socket, etc.
            self._okws.kill ()

        return rc

##=======================================================================

def main (argv):

    if False:
        print \
            TestCase.translate_data ("  {$ foo foo $}   more ${other} and more" )
        sys.exit (0)

    c = Config ()
    files = c.parseopts (argv[1:])
    r = RegTester (c)
    res = r.run (files)
    rc = -2
    if res: rc = 0
    sys.exit (rc)

##=======================================================================

main (sys.argv)
