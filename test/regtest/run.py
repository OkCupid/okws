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

class RegTestSkipped (Exception):
    def __init__ (self, s):
        self._s = s
    def __str__ (self):
        return repr (self._s)

##=======================================================================

INFO = 1
RESULT = 2
ERROR = 3

report_level = RESULT

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
"""usage: %s [-m] [-q|-v] [-c<casedir>] [-e] <case1> <case2> ...

    Run this testing harness from the top build directory.
 
    Flags
      -m, --command-line
          Run the command line pub (not the full server end-to-end test)

      -q, --quiet  
          Only make output on error
          
      -v, --verbose
          Output debug information and so on

      -e, --explain 
          don't run test case, just explain what it is

      -c <casedir>, --casedir=<dir>
          Supply a casedir to shorten the name of test cases; those
          cases given without absolute paths with be considered relative
          to the casedir. Typically set this equal to the top
          directory in the source code tree that relates to regtesting.
          Note: if no casedir is supplied, we assume the case dir is
          the same as the directory of sys.argv[0] --- the directory
          where run.py lives.

       -C, --no-casedir
          Do not assume a default case dir, as above

    Arguments:
        A list of files or directories with test cases in them;
        by default, use 'cases' if no arguments were supplied, which 
        is the directory (relative to the default casedir) in which
        OKWS keeps its regression tests.

""" % (progname)
    sys.exit (rc)

##=======================================================================

class Config:

    okws_config = "test/system/okws_config"
    pub_config = "test/system/pub_config"
    
    okld_exe = "okd/okld"
    
    # the stage-1 tester, upgrade this as okws3 becomes more mature.
    pub_exe = "pub/pub3stage1"

    _scratch = "regtest-scratch"
    static = "static"
    failed_files_dir = "regtest-failures"
    
    okld_test = [ okld_exe, "-f", okws_config ]
    pub_run = [ pub_exe, "-f", pub_config ]

    port = 8081
    hostname = "127.0.0.1"

    #-----------------------------------------

    def __init__ (self):
        self._explain_only = False
        self._jail_dir = None
        self._casedir = os.path.split (progname)[0]
        self._command_line = False
        pass

    #-----------------------------------------

    def explain_only (self):
        return self._explain_only

    #-----------------------------------------

    def command_line (self):
        return self._command_line

    #-----------------------------------------

    def parseopts (self, argv):
        short_opts = "mCc:eqv"
        long_opts = [ 'no-casedir',
                      'casedir=',
                      'explain',
                      'quiet',
                      'verbose',
                      'command-line' ]

        try:
            opts,args = getopt.getopt (argv, short_opts, long_opts)
        except getopt.GetoptError:
            usage (-1)

        global report_level
        report_level = RESULT

        for o,a in opts:

            if False: pass

            elif o in ("-C", "--no-casedir"):
                self._casedir = None
            elif o in ("-c", "--casedir"):
                self._casedir = a
            elif o in ("-e", "--explain"):
                self._explain_only = True
            elif o in ("-q", "--quiet"):
                report_level = ERROR
            elif o in ("-v", "--verbose"):
                report_level = INFO
            elif o in ('-m', '--command-line'):
                self._command_line = True

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

    def pub_file (self, f):
        """Calls pub v3 on a file and spits out the results to 
        standard output."""

        v = copy.copy (self.pub_run)
        v += [ f ]
        pipe = Popen(v, stdout=PIPE).communicate()
        return str (pipe[0])


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
            d = d + "/" + self._scratch
        return d

    #-----------------------------------------

    def jailed_casepath (self, f):
        return "/%s/%s.html" % (self._scratch, f)

    #-----------------------------------------

    def scratch_include_root(self):
        return "/" + self._scratch

    #-----------------------------------------

    def static_url (self):
        return self.url (self.static)

    #-----------------------------------------

    def scratch_url (self):
        return self.url ("%s/%s" % (self.static, self._scratch))

    #-----------------------------------------

    def service_url (self, s):
        return self.url (s)

    #-----------------------------------------

    def url (self, loc):
        return "http://%s:%d/%s" % ( self.hostname, self.port, loc)

    #-----------------------------------------

    def write_failure_file (self, nm, data):
        return self.write_debug_file (nm, data, "response")

    #-----------------------------------------

    def write_outcome_file (self, nm, data):
        return self.write_debug_file (nm, data, "expected")

    #-----------------------------------------

    def write_debug_file (self, nm, data, typ):
        d = self.failed_files_dir
        self.mkdir (d, "failures")
        fn = "%s/%s-%s" % (self.failed_files_dir, nm, typ)
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

class Outcome:

    def __init__ (self, dat):
        self._data = dat

    def __str__ (self):
        return "Expected Outcome (%s):\n%s" % (self.type (), self._data)

    def compare (self, input):
        raise NotImplementedError, "method Outcome::compare not implemented"

    def empty_is_ok (self):
        return False

    @classmethod
    def alloc (self, k):
        if k._outcome_exact:
            return OutcomeExact (k._outcome_exact)
        elif k._outcome:
            return OutcomeApprox (k._outcome)
        elif k._outcome_rxx:
            return OutcomeRegex (k._outcome_rxx)
        elif k._outcome_data:
            return OutcomeData (k._outcome_data)
        elif k._outcome_empty:
            return OutcomeEmpty ()
        else:
            raise RegTestError, "bad test case: no expected outcome given"

##-----------------------------------------------------------------------

class OutcomeEmpty (Outcome):
    def __init__ (self):
        Outcome.__init__ (self, None)
    def compare (self, input):
        return False
    def type (self):
        return "empty"
    def empty_is_ok (self):
        return True

##-----------------------------------------------------------------------

class OutcomeExact (Outcome):

    def __init__ (self, dat):
        Outcome.__init__ (self, dat)

    def type (self):
        return "exact"

    def compare (self, input):
        return (input == self._data)

##-----------------------------------------------------------------------

class OutcomeApprox (Outcome):

    def __init__ (self, dat):
        Outcome.__init__ (self, dat)

    def type (self):
        return "approx"

    def compare (self, input):
        return (input.split () == self._data.split ())

##-----------------------------------------------------------------------
        
class OutcomeRegex (Outcome):

    def __init__ (self, rxx):
        Outcome.__init__ (self, rxx)
        self._rxx = re.compile (rxx)

    def type (self):
        return "approx"

    def compare (self, input):
        return self._rxx.match (input)

##-----------------------------------------------------------------------

class OutcomeData (Outcome):

    def __init__ (self, dat):
        Outcome.__init__ (self, dat)

    def type (self):
        return "python-data";

    def compare (self, input):
        try:
            input_py = eval (input)
            return self._data == input_py
        except Exception, e:
            print " --> cannot convert to python: '%s'" % input.strip ()
            return False

##=======================================================================

class TestCase:
    """A class that corresponds to a single test case.  It's an abstract
    class since the subclasses needed to implement how to fetch the case ---
    via command-line interface, or via HTTP interface."""

    ##-----------------------------------------

    def __init__ (self, config, d = {}, raw = False):
        self._filedata = None
        self._filedata_inc = []
        self._desc = ""
        self._outcome = None
        self._outcome_exact = None
        self._outcome_rxx = None
        self._outcome_data = None
        self._service = None
        self._htdoc = None
        self._config = config 
        self._scratch_files = []
        self._result = False
        self._script_path = None
        self._custom_fetch = None
        self._nowss = False
        self._utf8json = False
        self._viserr = False
        self._no_error_page = False

        for k in d.keys ():
            setattr (self, "_" + k, d[k])

        if not raw:
            self._outcome_obj = Outcome.alloc (self)

            if not self._filedata \
                    and not self._htdoc \
                    and not self._service \
                    and not self._script_path \
                    and not self._custom_fetch:
                raise RegTestError, "bad test case: no input file given"

        if type (self._filedata) is list:
            self._filedata_inc = self._filedata[1:]
            self._filedata = self._filedata[0]

    ##----------------------------------------

    def is_local (self):
        return self._config.command_line ()

    ##----------------------------------------

    def config (self):
        return self._config

    ##----------------------------------------

    def custom_fetch (self):
        """The test case file might set a _custom_fetch attribute,
        which can be accessed via this method."""
        return self._custom_fetch

    ##----------------------------------------

    def translate_data (self, in_data):
        """Input data from Python files uses {$, $} and ${, instead
        of the equivalent commands with '%'.  This is to prevent us
        from doing lots of escaping when making test cases. This
        function makes the appropriate translation back."""

        rxx = re.compile ("({\\$|\\$}|\\${|\\$\\$|\\$\\[\\d+\\])")
        v_in = rxx.split (in_data)
        v_out = []
        i = 0
        inc_rxx = re.compile ("\\$\\[(\\d+)\\]")
        for e in v_in:
            if (i % 2 == 0):
                v_out += [ e ]
            elif e == "$$":
                v_out += [ "$" ]
            else:
                m = inc_rxx.match (e)
                if m:
                    v_out += [ self.include_path (int (m.group (1))) ]
                else:
                    v_out += [ char_subst (e, '$', '%') ]
            i += 1
        return ''.join (v_out)
        
    ##----------------------------------------
    
    def name (self):
        return char_subst (self._name, '/', '_')

    ##----------------------------------------

    def desc (self):
        return self._desc

    ##----------------------------------------

    def filepath (self):
        n = self.name ()
        d = self._config.scratch_dir ()
        return "%s/%s.html" % (d, n)

    ##----------------------------------------

    def filepath_inc (self, id):
        n = self.name ()
        d = self._config.scratch_dir ()
        return "%s/%s-%d.html" % ( d, n, id +1 )

    ##----------------------------------------

    def include_path (self, n):
        p = self._config.scratch_include_root ()
        ret = "%s/%s" % (p, self.name ())
        if (n > 0):
            ret += "-%d" % n
        ret += ".html";
        return ret

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
        return self.write_data_inner (out, dat)

    ##----------------------------------------

    def write_data_inc (self, n):
        """Those files that are included by the main test case should
        also be written out."""

        outfn = self.filepath_inc (n)
        dat = self.translate_data (self._filedata_inc[n])
        return self.write_data_inner (outfn, dat)

    ##----------------------------------------

    def write_data_inner (self, outfn, dat):
        f = open (outfn, "w")
        f.write (dat)
        self._scratch_files += [ outfn ]
        return outfn

    ##----------------------------------------

    def write_all (self):
        d = self.write_data ()
        for i in range (0, len (self._filedata_inc)):
            self.write_data_inc (i)
        return d

    ##----------------------------------------

    def cleanup (self):
        if self._result and self._scratch_files:
            for f in self._scratch_files:
                os.unlink (f)

    ##----------------------------------------

    def fetch (self):
        raise NotImplementedError, "pure virtual method"

    ##----------------------------------------

    def compare (self, txt):
        if txt and len (txt):
            res = self._outcome_obj.compare (txt)
        else:
            res = self._outcome_obj.empty_is_ok ()
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

    def explain (self):
        f = self.write_all ()
        incs = ""

        i = 0
        for f in self._filedata_inc:
            incs += "\n   -includes: " + self.filepath_inc (i)
            i += 1

        print \
"""---------------------------------------------------------------------
name: %s

desc: %s

input: %s%s

%s
""" % (self.name (), self.desc (), self.filepath (), incs, self._outcome_obj)

    ##----------------------------------------

    def run (self):
        cfg = self._config

        if cfg.explain_only ():
            self.explain ()
            return RESULT_OK

        self.write_all ()
        ret = RESULT_FAILED

        (status, data) = self.fetch ()

        if status == RESULT_OK:
            res = self.compare (data)
            if res:
                ret = status 
                self.report_success ()
                self.cleanup ()
            elif not data:
                self.report_failure ("empty reply")
            else:
                f = cfg.write_failure_file (self.name (), data)
                f2 = cfg.write_outcome_file (self.name (), 
                                             str (self._outcome_obj))
                
                txt = "data mismatch: diff -wB %s %s " % (f, f2)
                self.report_failure (txt)

        elif status == RESULT_SKIPPED:
            ret = status

        return ret
                
##=======================================================================

RESULT_OK = 0
RESULT_FAILED = 1
RESULT_SKIPPED = -1

##=======================================================================

class StatusCodes:
    OK = RESULT_OK
    FAILED = 1
    SKIPPED = -1

##=======================================================================

def query_str (d):
    s = ""
    if d:
        s = "?" + "=".join ([ "%s=%s" % p for p in d.items ()] )
    return s

##=======================================================================

class TestCaseGeneric (TestCase):
    """Test case in which the case determines all of the run logic
    (via run() function in the test case."""

    ##----------------------------------------

    def __init__ (self, config, d, fn):
        TestCase.__init__ (self, config, d, True) 
        self._run = fn

    ##----------------------------------------

    def run (self):
        # run the given run function, but with the test case wrapper
        # as a reference
        return self._run (self, StatusCodes)
        
##=======================================================================

class TestCaseRemote (TestCase):
    """Test case that's fetched via HTTP, with the full end-to-end
    tests enabled."""

    ##----------------------------------------

    def __init__ (self, config, d):
        TestCase.__init__ (self, config, d)

    ##----------------------------------------

    def file_url (self):

        n = None

        #
        # if the reg test specified the whole URL (after the leading
        # /), then fetch that here. Translate it first, to 
        # resolve $[i] as required.
        #
        if self._script_path:
            return self._config.url (self.translate_data (self._script_path))

        if self._filedata:
            n = self.name ()
        elif self._htdoc:
            n = self._htdoc

        opts = {}
        if self._nowss:         opts["nowss"] = 1
        if self._utf8json:      opts["utf8json"] = 1
        if self._viserr:        opts["viserr"] = 1
        if self._no_error_page: opts["no_error_page"] = 1
        
        if n:
            u = self._config.scratch_url ()
            r =  "%s/%s.html%s" % (u, n, query_str (opts))
        elif self._service:
            r = self._config.service_url (self._service)
        else:
            raise RegTestError, "cannot find appropriate URL for '%s'" % \
                self._name

        return r

    ##----------------------------------------

    def fetch (self):
        if self._custom_fetch:
            resp = self._custom_fetch (self)
        else:
            u = self.file_url ()
            resp = ''.join (urllib.urlopen (u))
        return (RESULT_OK, resp)

##=======================================================================

class TestCaseLocal (TestCase):
    """Test case that's run locally via the pub/pub3stage1 debugging tool."""

    def __init__ (self, config, d):
        TestCase.__init__ (self, config, d)

    ##----------------------------------------

    def fetch (self):
        if self._custom_fetch or self._script_path or self._htdoc:
            return (RESULT_SKIPPED, None)

        n = None
        if self._filedata:
            n = self.name ()
        if not n:
            raise RegTestError, "cannot find path for '%s'" % self._name

        jp = self._config.jailed_casepath (n)
        return (RESULT_OK, self._config.pub_file (jp))

##=======================================================================

class TestCaseLoader:

    ##----------------------------------------

    def __init__ (self, c):
        self._config = c
        self._iter = 0

    ##----------------------------------------

    def load_dir (self, name, full):
        v = []
        files = glob.glob ("%s/*.py" % full)
        files.sort ()
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
        self._iter += 1
        try:
            r = imp.find_module (full)
            mod = imp.load_module ('rtmod-%d' % self._iter, *r)
        except ImportError, e:
            raise RegTestError, "failed to load test case: %s" % full

        try:
            g = self.load_generic (name, full, mod)
            if g:
                v = [ g ]
            else:
                v = self.load_cases (name, full, mod)
                if not v:
                    v = [ self.load_single_case (name, full, mod) ]
        except RegTestError, e:
            myerr (full, e)
            raise RegTestError, "cannot load test case file: %s" % full

        return v
            
    ##----------------------------------------

    def make_test_case (self, d):
        ret = None
        if self._config.command_line (): 
            ret = TestCaseLocal (self._config, d)
        else:
            ret = TestCaseRemote (self._config, d)
        return ret

    ##----------------------------------------

    def make_generic_case (self, d, fn):
        ret = TestCaseGeneric (self._config, d, fn)
        return ret

    ##----------------------------------------

    def load_generic (self, name, full, mod):
        """Load a generic single test case, that implements its
        own run() function."""

        ret = None
        fn = None
        try:
            fn = getattr (mod, "run")
        except AttributeError, e:
            pass
        if fn:
            d = { "name" : name, "fullpath" : full }
            ret = self.make_generic_case (d, fn)
        return ret

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

        return self.make_test_case (d)

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
                v += [ self.make_test_case (c) ]
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
        self._pid = -1

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
            time.sleep (2)

    ##-----------------------------------------

    def kill (self):
        if self._pid > 0:
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

        
    def run_server (self):
        return not self._config.command_line ()

    ##-----------------------------------------

    def run (self, files):

        rc = True
        server = False

        eo = self._config.explain_only ()

        if not eo and self.run_server ():
            server = True
            self._okws.run ()

        if not files:
            files = [ "cases" ]

        try:
            v = self._loader.load (files)
            ok = 0
            skipped = 0
            tot = len (v)
            for c in v:
                code = c.run ()
                if code == RESULT_OK:
                    ok += 1
                elif code == RESULT_SKIPPED:
                    skipped += 1
            rc = (tot == (ok + skipped))

            if eo:
                pass
            elif rc: 
                if skipped == 0:
                    msg ("++ All %d tests passed. YES!!" % tot, RESULT)
                else:
                    msg ("++ All %d tests passed (with %d skipped)" \
                             % (tot, skipped), RESULT)
            else:
                msg ("-- Only %d/%d tests passed (with %d skipped). Booo." \
                         % (ok, tot - skipped, skipped), ERROR)
            
        finally:

            if server:
                # make sure we alway clean up after ourselves, that way
                # OKWS won't leave a stale socket, etc.
                self._okws.kill ()

        return rc

##=======================================================================

def main (argv):
    c = Config ()
    files = c.parseopts (argv[1:])
    r = RegTester (c)
    res = r.run (files)
    rc = -2
    if res: rc = 0
    sys.exit (rc)

##=======================================================================

main (sys.argv)
