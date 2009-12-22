#!/usr/bin/env python

'''pub2-upgrade.py

     A tranlsator script that peforms the following upgrades:

        ${x} -> %{x}
        setl,setle -> locals
        set -> globals
        for (i, l) { i.iter } -> for (i, decorate (l)) { i.iter }

     Meant as a stage in a series of translations you can apply to
     purge old-style pub constructs...
'''


__author__ = 'max@okcupid.com'
__version__ = '2.1'

import ply
import ply.lex
import ply.yacc
import sys
import getopt
import os
import re
import os.path

##=======================================================================

progname = sys.argv[0]

def usage ():
    print \
"""usage: %(prog)s [-d] [-x<regex>] <files>
      - specify no args to act a filter
      - specify files to rewrite
      - specify directories to explore (looks for html/conf files)
           (if -x is specified, only do those files that match the regex)""" \
        % { "prog" : progname }
    

##=======================================================================

def nlcount (s):
    r = 0
    for c in s:
        if c == '\n':
            r += 1
    return r

##=======================================================================

PUB_TOKENS = [ 

    'LBP',
    'RBP',
    'LBLB',
    'RBRB',
    'LB',
    'RB',
    'CODE'

]

##=======================================================================

class PubError (Exception):
    def __init__ (self, e):
        self._err = e
    def __str__ (self):
        return self._err

##=======================================================================

class PubLexer (object):

    def __init__ (self, **kwargs):
        """Ply magic to turn this class into a scanner
        given the class variables we set below."""

        self.lexer = ply.lex.lex (module = self, **kwargs)
        self._lineno = 1

    def set_filename (self, n):
        self._filename = n

    # Tokens
    tokens = PUB_TOKENS

    # The default state is HTML mode.  We transition to pub mode within
    # {% tags
    states = ( ("pub",    "exclusive"),  )
              
    def t_LBP (self, t):
        r'\{%'
        t.lexer.push_state ('pub')
        return t

    def t_RBRB (self, t):
        r'\}\}'
        t.lexer.pop_state ()
        return t

    # Note a huge difference between this and flex.  In flex, the
    # order of the rules doesn't matter --- it's just longest-match
    # first.  In this jam, the order does matter, and this rules
    # needs to show up last.
    def t_CODE (self, t):
        r'([^{}]+|[{}])'
        t.lexer.lineno += nlcount (t.value)
        return t

    def t_pub_RBP (self, t):
        r'%\}'
        t.lexer.pop_state ()
        return t

    def t_pub_LBLB (self, t):
        r'\{\{'
        t.lexer.push_state ('INITIAL')
        return t

    t_pub_RB = r'\}'
    t_pub_LB = r'\{'

    def t_pub_CODE (self, t):
        r'([^{}%]+|[%])'
        t.lexer.lineno += nlcount (t.value)
        return t
        
    def t_ANY_error(self, t):
        raise PubError, \
            "Illegal character '%s' (@ %s)" %  (t.value[0], t)

    def tokenize(self, data, *args, **kwargs):
        '''Invoke the lexer on an input string an return the list of tokens.

        This is relatively inefficient and should only be used for
        testing/debugging as it slurps up all tokens into one list.

        Args:
        data: The input to be tokenized.
        Returns:
        A list of LexTokens
        '''
        self.lexer.input(data)
        tokens = list()
        while True:
            token = self.lexer.token()
            if not token:
                break
            tokens.append(token)
        return tokens

##=======================================================================

class Node:
    def __init__ (self):
        pass

##-----------------------------------------------------------------------

class Block (Node):

    def __init__ (self, els, open, close):
        self._els = els
        self._open = open
        self._close = close

    def __str__ (self):
        ret = ''.join ( [self._open] + [str (e) for e in self._els] + \
                            [self._close])
        return ret

    def for_loop_decorate (self, nxt = None):
        ret = False
        for (i,e) in enumerate (self._els):
            nxt = None
            if (i + 1 < len (self._els)):
                nxt = self._els[i+1]
            if e.for_loop_decorate (nxt):
                ret = True
        return ret

##-----------------------------------------------------------------------

class HtmlBlock (Block):
    def __init__ (self, els, open = "", close = ""):
        Block.__init__ (self, els, open, close)

##-----------------------------------------------------------------------

class PubBlock (Block):
    def __init__ (self, els, open, close):
        Block.__init__ (self, els, open, close)

##-----------------------------------------------------------------------

class Code (Node):
    def __init__ (self):
        self._v = []
        self._s = None
    def __str__ (self):
        if self._s is None:
            self._s = ''.join (self._v)
        return self._s
    def add (self, s):
        self._v += [ s ]
    def for_loop_decorate (self, nxt = None):
        ret = False
        b = r'(?P<pre>.*)' + \
            r'for\s*\((?P<iter>[a-zA-Z0-9_]+)\s*,\s*(?P<vec>.*)\s*\)' + \
            r'(?P<post>\s*)'
        x = re.compile (b)
        s = str (self)
        m = x.match (s)
        if nxt and m:
            d = m.groupdict ()
            i = r'\b' + d["iter"] + "\\.(first|last|count|iter|even|odd)" + \
                r'\b'
            ix = re.compile (i)
            if ix.search (str (nxt)):
                self._s = \
                    "%(pre)sfor (%(iter)s, decorate (%(vec)s))%(post)s" % d
                self._v = [ self._s ]
                ret = True
        return ret
    

##=======================================================================

class PubParser (object):

    def __init__(self, lexer=None, **kwargs):
        '''Constructs the JsonParser based on the grammar contained herein.

        Successful construction builds the ply.yacc instance and sets
        self.parser.
        
        Args:
        lexer: A ply.lex or JsonLexer instance that will produce JSON_TOKENS.
        '''
        if lexer is not None:
            if isinstance(lexer, PubLexer):
                self.lexer = lexer.lexer
            else:
                # Assume that the lexer is a ply.lex instance or similar
                self.lexer = lexer
        else:
            self.lexer = PubLexer().lexer
        self.parser = ply.yacc.yacc(module=self, **kwargs)

    # interface between parse and lexer
    tokens = PUB_TOKENS

    # Define the parser
    def p_file(self, p):
        '''file : block_inner '''
        p[0] = HtmlBlock (p[1])

    def p_block (self, p):
        '''block : html_block 
                 | pub_block 
                 | pub_inner_block'''
        if p[1]: p[0] = p[1]
        elif p[2]: p[0] = p[2]
        else: p[0] = p[3]

    def p_html_block (self, p):
        '''html_block : LBLB block_inner RBRB'''
        p[0] = HtmlBlock (p[2], p[1], p[3])

    def p_pub_block (self, p):
        '''pub_block : LBP block_inner RBP'''
        p[0] = PubBlock (p[2], p[1], p[3])

    def p_pub_inner_block (self, p):
        '''pub_inner_block : LB block_inner RB'''
        p[0] = PubBlock (p[2], p[1], p[3])

    def p_block_inner (self, p):
        '''block_inner : code 
                       | block_inner block code'''
        if len (p) == 2: 
            p[0] = [ p[1] ]
        else:
            p[0] = p[1] + [ p[2], p[3] ]

    def p_code (self, p):
        '''code : 
                | code CODE'''
        if len (p) == 1:
            p[0] = Code ()
        else:
            p[1].add (p[2])
            p[0] = p[1]

    ##----------------------------------------

    def p_error(self, p):
        where = "EOF"
        if p: where = "'%s'" % p
        ret = "Syntax error at %s" % where
        raise PubError, ret
    
    ##----------------------------------------

    def set_filename (self, n):
        self._filename = n
        #self.lexer.set_filename (n)

    # Invoke the parser
    def parse(self, data, lexer=None, *args, **kwargs):
        '''Parse the input JSON data string into a python data structure.
        
        Args:
        data: An input data string
        lexer:  An optional ply.lex instance that overrides the default lexer.
        Returns:
        A python dict or list representing the input JSON data.
        '''
        if lexer is None:
            lexer = self.lexer
        return self.parser.parse(data, lexer=lexer, *args, **kwargs)

##=======================================================================

def problem (s):
    if s[-1] != '\n': s += '\n'
    sys.stderr.write (s)

##-----------------------------------------------------------------------

def info (s):
    problem (s)

##=======================================================================

class ForLoopDecorator:

    def __init__ (self):
        pass

    def convert (self, obj):
        return obj.for_loop_decorate ()

##=======================================================================

class Runner:
    
    ##----------------------------------------

    def __init__ (self, converter = None, debug = False):
        self._parser = PubParser ()
        self._debug = debug
        self._regex = r'.*\.(html|conf|js)$'
        self._exclude = r'\.svn'
        self._verbose = False
        self._converter = converter

    ##----------------------------------------

    def parse (self, data):
        '''Run a given file through the converter.'''
        if self._debug:
            lx = PubLexer ()
            raw = '\n'.join ([str (x) for x in lx.tokenize (data)] )
            sys.stderr.write (raw)

        obj = self._parser.parse (data)

        # returns None if there were no changes made
        ret = None
        if self._converter and self._converter.convert (obj):
            ret = str (obj)

        return ret

    ##----------------------------------------

    def run_stream (self, infh, outfh, filename):
        data = infh.read ()
        try:
            self._parser.set_filename (filename)
            outdat = self.parse (data)
            if outdat:
                outfh.write (outdat)
                return 1
            else:
                # in this case, no data means either that the file
                # was empty, or that there were no changes made to
                # the file.
                return 0
        except PubError, e:
            problem ("Parse Error: %s:%s" % (filename, e))
            return -1

    ##----------------------------------------

    def run_file (self, filename):

        tmp = filename + ".p1u-tmp"
        outfh = None
        ret = False

        if self._verbose:
            info ("++ run file: %s" % filename)

        try:
            infh = open (filename, "r")
            outfh = open (tmp, "w")
            rc = self.run_stream (infh, outfh, filename)
            ret = (rc >= 0)
            
            # only replace the file if a change was made
            if rc > 0:
                outfh.close ()
                infh.close ()
                os.rename (tmp, filename)
                tmp = None
                info ("%s: changed!" % filename)

        except IOError, e:
            problem ("%s: %s" % (filename, e))
    
        if outfh and tmp:
            outfh.close ()
            os.unlink (tmp)

        return ret
        
    ##----------------------------------------

    def run_filter (self):
        return self.run_stream (sys.stdin, sys.stdout, "<stdin>")

    ##----------------------------------------

    def run_dir (self, a):
        
        rxx = re.compile (self._regex)
        erxx = re.compile (self._exclude)
        for root, dirs, files in os.walk (a):
            for file in files:
                file = os.path.join (root, file)
                if rxx.match (file) and not erxx.search (file):
                    if not self.run_file (file):
                        return False
        return True

    ##----------------------------------------

    def run_file_name (self, a):
        ret = False 
        if os.path.isfile (a):
            ret = self.run_file (a)
        elif os.path.isdir (a):
            ret = self.run_dir (a)
        else:
            problem ("%s: not a file or directory" % a)
        return ret

    ##----------------------------------------

    def run (self):
        ret = True
        if len (self._args):
            for a in self._args:
                if not self.run_file_name (a):
                    ret = False
                    break
        else:
            ret = self.run_filter ()
        return ret

    ##----------------------------------------

    def config (self, argv):

        short_opts = "dx:v"
        long_opts = [ "debug", "regex=", "verbose" ]
                             
        try:
            opts, args = getopt.getopt (argv[1:], short_opts, long_opts)
        except getopt.GetoptError:
            usage ()

        for o,a in opts:

            if False: 
                pass
            elif o in ("-d", "--debug"): 
                self._debug = True
            elif o in ("-x", "--regex"):
                self._regex = a
            elif o in ("-v", "--verbose"):
                self._verbose = True
            else: 
                usage ()

        self._args = args

    ##----------------------------------------

##=======================================================================

def main (argv):
    runner = Runner (ForLoopDecorator ())
    runner.config (argv)
    ok = runner.run ()
    rc = 0
    if not ok: rc = 5
    sys.exit (rc)

##=======================================================================

if __name__ == '__main__':
    main(sys.argv)

