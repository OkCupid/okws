#!/usr/bin/env python

'''pub1-upgrade.py

     A tranlsator script that purges your code of the follwing pub1
     constructs:

        <!--#switch -->  / {% switch %}
        <!--#set --> 
        <!--#load -->
        <!--#include -->

     Meant as a stage in a series of translations you can apply to
     purge old-style pub constructs...
'''


__author__ = 'max@okcupid.com'
__version__ = '0.1'

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

PUB1_TOKENS = [ 'PTSWITCH', 
                'PTSET', 
                'PTINCLUDE', 
                'PTLOAD',
                'HTML',
                'COMMA', 
                'LPAREN', 'RPAREN', 
                'DQUOTE', 'SQUOTE' ,
                'INTEGER',
                'DLBRACE', 'DRBRACE', 
                'PERLARROW', 'COLON', 'EQUALS',
                'LBRACE', 'RBRACE', 
                'VAR',
                'PCLOSE',
                'STRING',
                'ARRAY_OPEN',
                'PASS_LBRACE',
                'PASS_RBRACE',
                'TLBRACKET', 'TRBRACKET', 'COMMENT',
                'RANGE',
                'NULL' ]

##=======================================================================

class Pub1Error (Exception):
    def __init__ (self, e):
        self._err = e
    def __str__ (self):
        return self._err

##=======================================================================

class Pub1Lexer (object):

    def __init__ (self, **kwargs):
        """Ply magic to turn this class into a scanner
        given the class variables we set below."""

        for t in [ "PTSWITCH", "PTSET", "PTINCLUDE", "PTLOAD", "HTML" ]:
            setattr (self, "t_nhtml_" + t, getattr (self, "t_" + t))

        self.lexer = ply.lex.lex (module = self, **kwargs)
        self._lineno = 1

    def set_filename (self, n):
        self._filename = n

    # Tokens
    tokens = PUB1_TOKENS

    # The default state is HTML mode.  We transition to pub mode within
    # <!--# tags
    states = ( ("sstring", "exclusive"),
               ("dstring", "exclusive"),
               ("pub",    "exclusive"),
               ("nhtml",  "exclusive"),
               ("comment", "exclusive") )
    
    # tokens found in HTML mode that force a push to pub mode
    def t_PTSWITCH(self, t):
        r'(<!--\#|\{%)[ \t]*switch'
        t.lexer.push_state ('pub')
        return t
    def t_PTSET(self, t):
        r'<!--\#[ \t]*set'
        t.lexer.push_state ('pub')
        return t
    def t_PTINCLUDE(self, t):
        r'<!--\#[ \t]*include'
        t.lexer.push_state ('pub')
        return t
    def t_PTLOAD(self, t):
        r'<!--\#[ \t]*load'
        t.lexer.push_state ('pub')
        return t

    def t_PASS_LBRACE (self, t):
        r'\{'
        t.lexer.push_state ('INITIAL')
        return t
    def t_PASS_RBRACE (self, t):
        r'\}'
        try:
            t.lexer.pop_state ()
        except IndexError, e:
            raise Pub1Error, "Unbalanced '}' (@ %s)" % t
        return t

    def t_nhtml_DRBRACE(self, t):
        r'\}\}'
        t.lexer.pop_state ()
        return t
    def t_nhtml_PASS_LBRACE (self, t):
        r'\{'
        t.lexer.push_state ('INITIAL')
        return t
    def t_nhtml_PASS_RBRACE (self, t):
        r'\}'
        t.lexer.pop_state ()
        return t

    # Note a huge difference between this and flex.  In flex, the
    # order of the rules doesn't matter --- it's just longest-match
    # first.  In this jam, the order does matter, and this rules
    # needs to show up last.
    def t_HTML (self, t):
        r'([^{<%}]+|[<%])'
        t.lexer.lineno += nlcount (t.value)
        return t

    # tokens found in PUB mode
    t_pub_COMMA      = r','
    t_pub_RPAREN     = r'\)'
    t_pub_LPAREN     = r'\('
    t_pub_INTEGER    = r'[+-]?([0-9]+|0x[0-9a-f]+)'
    t_pub_PERLARROW  = r'=>'
    t_pub_COLON      = r':'
    t_pub_EQUALS     = r'='
    t_pub_LBRACE     = r'\{'
    t_pub_RBRACE     = r'\}'
    t_pub_ARRAY_OPEN = r'(u_)?(int|char)(16|32|64)?(_t)?\('
    t_pub_NULL       = r'NULL'

    t_pub_ignore = ' \t'

    def t_pub_RANGE(self, t):
        r'r\#\[\d+-\d+\]\#'
        return t

    def t_pub_VAR(self, t):
        r'[a-zA-Z_][a-zA-Z_0-9.]*'
        if t.value == "NULL":
            t.type = "NULL"
        return t

    # transitions out of pub1 mode into other modes
    def t_pub_DLBRACE(self, t):
        r'\{\{'
        t.lexer.push_state ('nhtml')
        return t
    def t_pub_PCLOSE(self, t):
        r'(-->|%})'
        t.lexer.pop_state ()
        return t
    def t_pub_DQUOTE(self, t):
        '"'
        t.lexer.push_state ('dstring')
        return t
    def t_pub_SQUOTE(self, t):
        "'"
        t.lexer.push_state ('sstring')
        return t

    def t_pub_TLBRACKET(self, t):
        r'\[\[\['
        t.lexer.push_state ("comment")
        return t

    t_sstring_STRING = r'([^\'\\]+|\\(.))'

    def t_sstring_SQUOTE(self, t): 
        r'\''
        t.lexer.pop_state ()
        return r

    t_dstring_STRING = r'([^"\\]+|\\(.))'

    def t_dstring_DQUOTE(self, t):
        r'"'
        t.lexer.pop_state ()
        return t

    def t_comment_TRBRACKET(self, t):
        r'\]\]\]'
        t.lexer.pop_state ()
        return t

    def t_comment_COMMENT(self, t): 
        r'([^\]]+|\])'
        t.lexer.lineno += nlcount (t.value)
        return t

    def t_ANY_error(self, t):
        raise Pub1Error, \
            "Illegal character '%s' (@ %s)" %  (t.value[0], t)

    def t_ANY_newline(self, t):
        r'\n+'
        t.lexer.lineno += len(t.value)

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


class OutputEngine (object):
    """Outputs the translated file in a way that's formatted as nicely
    as possible."""

    ##----------------------------------------

    tabwidth = 4

    ##----------------------------------------

    class Frame:
        def __init__ (self):
            self.hit_text ()
        def hit_text (self):
            self._last_was_text = True
            self._code_indent = 0
            self._n_tabs = 0

    ##----------------------------------------

    def __init__ (self):
        self._lines = None
        self._stack = [ self.Frame () ]
        self._hit = False
        self._swallow_newline = False
        self._inlines = [ False ]

    ##----------------------------------------

    def push_inline (self):
        self._inlines += [ True ]
    def pop_inline (self):
        self._inlines = self._inlines[0:-1]

    ##----------------------------------------

    def hit (self):
        self._hit = True

    ##----------------------------------------

    def back (self):
        return self._stack[-1]

    ##----------------------------------------

    def append_txt (self, txt):
        if self._lines is None:
            self._lines = [ "" ]
        self._lines[-1] += txt

    ##----------------------------------------

    def text_output (self, txt):
        if txt and len(txt):
            lines = txt.split ('\n')
            if self._lines is None:
                self._lines = lines
            elif len (lines):
                self.append_txt (lines[0])
                self._lines += lines[1:]
            self.back ().hit_text ()
            self._swallow_newline = False

    ##----------------------------------------

    def __str__ (self):
        return '\n'.join (self._lines)

    ##----------------------------------------

    def adjust_tab (self, indent):
        self.back ()._code_indent += self.tabwidth * indent

    ##----------------------------------------

    def reset_lastlinelen (self):
        if self._lines:
            tabs = 0
            spaces = 0
            l = self._lines[-1]
            for c in l:
                if c == '\t': tabs += 1
                else: spaces += 1
            self.back ()._code_indent = spaces
            self.back ()._n_tabs = tabs

    ##----------------------------------------

    def changes_made (self):
        return self._hit

    ##----------------------------------------

    def swallow_newline (self):
        self._swallow_newline = True

    ##----------------------------------------

    def code_output (self, txt, indent = 0):
        self._hit = True
        if (indent < 0):
            self.adjust_tab (indent)
        if self.back ()._last_was_text:
            self.reset_lastlinelen ()
            self.back ()._last_was_text = False
            self.append_txt (txt)
        elif self._swallow_newline:
            self._swallow_newline = False
            self.append_txt (" " + txt)
        elif self._inlines[-1]:
            self.append_txt (" " + txt)
        else:
            self._lines += [ self.space () + txt ]
        if (indent > 0):
            self.adjust_tab (indent)

    ##----------------------------------------

    def atom_output (self, txt):
        self.append_txt (txt)

    ##----------------------------------------

    def space (self):
        return "\t" * self.back ()._n_tabs + \
            " " * self.back ()._code_indent 

    ##----------------------------------------

    def push_frame (self):
        self._stack.append (self.Frame ())

    ##----------------------------------------

    def pop_frame (self):
        self._stack = self._stack[0:-1]
        
##=======================================================================

class AstNode:
    def __init__ (self):
        pass
    def output_in_switch (self, oe):
        self.output (oe)
    def html_text (self):
        return None
    def output (self, oe):
        oe.output (str (self))
    def isnull (self):
        return False
    def is_empty_str (self):
        return False
    def is_list (self):
        return False
    def is_dict (self):
        return False
    def is_empty (self):
        return False
    def if_key (self, key):
        raise Pub1Error, \
            "Pub token does not make a valid if key: %s -> '%s'" % \
            (key, str (s))

##-----------------------------------------------------------------------

class HtmlText (AstNode):
    def __init__ (self, html = None):
        self._html = html
    def output (self, oe):
        oe.text_output (self._html)
    def output_in_switch (self, oe):
        self.output (oe)
    def __str__ (self):
        if self._html: return self._html
        else: return ''
    def html_text (self):
        return self._html
    def is_empty (self):
        return len (self._html) == 0

##-----------------------------------------------------------------------

class HtmlBlock (AstNode):
    def __init__ (self, blocks):
        AstNode.__init__ (self)
        self._blocks = blocks
    def output (self, oe):
        for b in self._blocks:
            b.output (oe)
    def is_empty (self):
        for b in self._blocks:
            if not b.is_empty ():
                return False
        return True

##-----------------------------------------------------------------------

class PubNode (AstNode):
    def __init__ (self, start = 0, end = 0):
        AstNode.__init__ (self)
        self._startln = start
        self._endln = end
    def push_inline (self, oe):
        if self._startln == self._endln:
            oe.push_inline ()
    def pop_inline (self, oe):
        if self._startln == self._endln:
            oe.pop_inline ()
    def can_switch (self):
        return True

##-----------------------------------------------------------------------

class NestedHtml (AstNode):
    def __init__ (self, h):
        AstNode.__init__ (self)
        self._h = h
    def output (self, oe):
        oe.push_frame ()
        oe.atom_output ("{{")
        self._h.output (oe)
        oe.atom_output ("}}")
        oe.pop_frame ()
    def is_empty (self):
        return self._h.is_empty ()

##-----------------------------------------------------------------------

class PubInt (PubNode):
    def __init__ (self, v):
        PubNode.__init__ (self)
        self._v = v
    def output (self, oe):
        oe.atom_output (str (self))
    def __str__ (self):
        return str (self._v)
    def if_key (self, key):
        return "%s == %d" % (key, self._v)

##-----------------------------------------------------------------------

class PubString (PubNode) :
    def __init__ (self, v):
        PubNode.__init__ (self)
        self._v = v
    def __str__ (self):
        return '"' + str (self._v) + '"'
    def output (self, oe):
        oe.atom_output (str (self))
    def output_in_switch (self, oe):
        t = '{ include (%s) }' % str (self)
        oe.atom_output (t)
    def if_key (self, key):
        return "%s == %s" % (key, self)
    def is_empty_str (self):
        return len (self._v) == 0
    def is_empty (self): 
        return self.is_empty_str ()

##-----------------------------------------------------------------------

class PubSwitch (PubNode):
    def __init__ (self, key, cases, start, end):
        PubNode.__init__ (self, start, end)
        self._key = key
        self._cases = cases

    ##----------------------------------------

    def output(self, oe):
        self.sort_cases ()
        if self.output_null_check (oe):
            pass
        elif self.can_switch() and len (self._cases) > 2:
            self.output_switch3 (oe)
        else:
            self.output_ifelse (oe)

    ##----------------------------------------

    def output_null_check (self, oe):
        """Handle a special, common case: using <!--#switch--> to test
        if a string is either empty or NULL, or both."""
        empties = []
        defaults = []
        nulls = []

        fail = False
        ret = False 

        for c in self._cases:
            if len (c) != 2:
                fail = True
            elif not c[0]:
                defaults += [ c ]
            elif c[0].is_empty_str() and not c[1]:
                empties += [ c ]
            elif c[0].isnull () and not c[1]:
                nulls += [ c ]
            else:
                fail = True

        if fail:
            pass
        elif len (nulls) == 1 and len (empties) <= 1 and \
                len (defaults) == 1:
            self.push_inline (oe)
            oe.code_output ("{%", 1)
            test = "%s" % self._key
            if len(empties) == 0:
                test = "!isnull (%s)"  % self._key
            oe.code_output ("if (%s) " % test, 0)
            defaults[0][1].output_in_switch (oe)
            oe.code_output ("%}", -1)
            ret = True
            self.pop_inline (oe)
            
        return ret

    ##----------------------------------------

    def output_switch3 (self, oe):
        oe.code_output ("{%", 1);
        oe.code_output ("Switch21Tmp (%s) {" % self._key , 1) 
        for c in self._cases:
            if len (c) == 0:
                oe.code_output ("case () {}", 0)
            else:
                key = c[0]

                if key: key = ' (%s)' % key
                else: key = ''

                oe.code_output ('case%s ' % key, 0)
                if len (c) > 1 and c[1]: 
                    c[1].output_in_switch (oe)
                else:
                    oe.atom_output ("{}")
        oe.code_output ("}", -1)
        oe.code_output ("%}", -1)

    ##----------------------------------------

    def can_switch (self):
        for c in self._cases:
            if c[0] and not c[0].can_switch ():
                return False
        return True

    ##----------------------------------------

    def sort_cases (self):
        nulls = []
        newcases = []
        for c in self._cases:
            if len (c) == 0:
                pass
            elif not c[0]:
                nulls += [ c ]
            else:
                newcases += [ c ]
        if len (nulls) > 1:
            raise Pub1Error, \
                "%d: Multiple default cases for switch" % self._startln
        if len (newcases) == 0:
            raise Pub1Error, \
                "%d: No non-default cases for switch" % self._startln
        self._cases = newcases + nulls

    ##----------------------------------------

    def output_ifelse (self, oe):
        self.push_inline (oe)
        oe.code_output ("{%", 1)
        for c in enumerate (self._cases):
            self.output_case (oe, c[1], c[0])
        oe.code_output ("%}", -1)
        self.pop_inline (oe)

    ##----------------------------------------

    def output_if_key (self, case):
        ret = None
        if case[0]:
            ret = case[0].if_key (self._key)
        return ret

    ##----------------------------------------

    def output_case (self, oe, case, index):
        keywd = "elif"
        if index == 0: keywd = "if"
        key = self.output_if_key (case)

        empty_body = len(case) == 1 or not case[1] or case[1].is_empty ()

        if key:
            oe.code_output ("%s (%s) " % ( keywd, key) )
        elif not empty_body:
            oe.code_output ("else ")
        if not empty_body:
            case[1].output_in_switch (oe)
        elif key:
            oe.atom_output ("{}")
        
##-----------------------------------------------------------------------

class PubList (list):

    def __init__ (self, l):
        list.__init__ (self, l )

    def is_list (self): return True
    def open_char (self): return '['
    def close_char (self): return ']'

    def output (self, oe):
        first = True 
        oe.atom_output (self.open_char ())
        for i in self:
            if first: first = False
            else: oe.atom_output (", ")
            i.output (oe)
        oe.atom_output (self.close_char ())

##-----------------------------------------------------------------------

class PubArgList (PubList):

    def __init__ (self, l):
        PubList.__init__ (self, l)

    def open_char (self): return '('
    def close_char (self): return ')'

##-----------------------------------------------------------------------

class PubDict:

    def __init__ (self, i = None):
        self._items = [ ]
        if i:
            self._items += [ i ]


    def add (self, i):
        self._items += [ i ]

    def is_dict (self): return True

    def output (self, oe):
        oe.code_output ("{", 1)
        first = True 
        for i in self._items:
            if first:
                first = False
            else:
                oe.atom_output (",")
            oe.code_output ( str (i[0]) + " : ", 0)
            i[1].output (oe)
        oe.code_output ("}", -1)

##-----------------------------------------------------------------------

class PubNull (PubNode):
    def __init__ (self):
        PubNode.__init__ (self)
    def __str__ (self):
        return "null"
    def isnull (self):
        return True
    def if_key (self, key):
        return "isnull (%s)" % (key)

##-----------------------------------------------------------------------

class PubSet (PubNode):
    def __init__ (self, arg, start, end):
        PubNode.__init__ (self, start, end)
        self._arg = arg

    def output (self, oe):
        if self._arg.is_list ():
            for a in self._arg:
                self.output_dict (oe, a)
        else:
            self.output_dict (oe, self._arg)

    def output_dict (self, oe, d):
        if not d.is_dict ():
            raise Pub1Error, \
                "%d: Expected dict arguments to set" % self._startln
        self.push_inline (oe)
        oe.code_output ("{%", 1)
        oe.code_output ("set", 0)
        oe.swallow_newline ()
        d.output (oe)
        oe.code_output ("%}", -1)
        self.pop_inline (oe)

##-----------------------------------------------------------------------

class PubInclude (PubNode):
    def __init__ (self, args, load, start, end):
        PubNode.__init__ (self, start, end)
        self._args = args
        self._load = load

    def output (self, oe):
        self.push_inline (oe)
        oe.code_output ("{%", 1)
        nm = "include "
        if self._load: nm = "load "
        oe.code_output (nm, 0)
        oe.swallow_newline ()
        self._args.output (oe)
        oe.code_output ("%}", -1)
        self.pop_inline (oe)

##-----------------------------------------------------------------------

class PubRange (PubNode):
    rxx = re.compile (r'r#\[(\d+)-(\d+)\]#')
    def __init__ (self, raw, lineno):
        PubNode.__init__ (self, lineno, lineno)
        m = self.rxx.match (raw)
        if not m:
            raise Pub1Error, "%d: bad pub range: %s" % (lineno, raw)
        self._lower = int (m.group (1))
        self._upper = int (m.group (2))

    def can_switch (self):
        return False

    def if_key (self, key):
        return "%s <= %d && %s >= %d" % (key, self._upper, key, self._lower)

##-----------------------------------------------------------------------

class PubComment (PubNode):
    def __init__ (self, l):
        self._list = l
    def output (self, oe):
        oe.swallow_newline (oe)
        oe.code_output ('[[[' + ''.join (self._list) + ']]]', 0)

##=======================================================================

class Pub1Parser (object):

    def __init__(self, lexer=None, **kwargs):
        '''Constructs the JsonParser based on the grammar contained herein.

        Successful construction builds the ply.yacc instance and sets
        self.parser.
        
        Args:
        lexer: A ply.lex or JsonLexer instance that will produce JSON_TOKENS.
        '''
        if lexer is not None:
            if isinstance(lexer, Pub1Lexer):
                self.lexer = lexer.lexer
            else:
                # Assume that the lexer is a ply.lex instance or similar
                self.lexer = lexer
        else:
            self.lexer = Pub1Lexer().lexer
        self.parser = ply.yacc.yacc(module=self, **kwargs)

    # interface between parse and lexer
    tokens = PUB1_TOKENS

    # Define the parser
    def p_file(self, p):
        '''file : blocks'''
        p[0] = HtmlBlock (p[1])

    def p_blocks (self, p):
        '''blocks : html
                  | blocks pub html'''
        if len (p) == 2: 
            p[0] = [ p[1] ]
        else:
            p[0] = p[1] + [ p[2], p[3] ]

    def p_html (self, p):
        '''html : html_frags'''
        p[0] = HtmlText (''.join (p[1]))

    def p_html_frags (self, p):
        '''html_frags : 
                      | html_frags html_frag'''
        if len (p) == 1: p[0] = []
        else:
            p[1].append (p[2])
            p[0] = p[1]

    def p_html_frag (self, p):
        '''html_frag : HTML
                     | PASS_LBRACE
                     | PASS_RBRACE'''
        p[0] = p[1]
        
    def p_pub (self, p):
        '''pub : switch 
               | set 
               | include 
               | load 
               | comment '''
        p[0] = p[1]

    def p_comment (self, p):
        '''comment : TLBRACKET comment_frags TRBRACKET'''
        p[0] = PubComment (t[1])

    def p_comment_frags (self, p):
        '''comment_frags : COMMENT
                         | comment_frags COMMENT'''
        if len (p) == 2: p[0] = [ p[1] ]
        else:
            p[1].append ( p[2] )
            p[0] = p[1]

    def p_set (self, p):
        '''set : PTSET arg PCLOSE'''
        p[0] = PubSet (p[2], p.lineno (1), p.lineno (3))

    def p_include (self, p):
        '''include : PTINCLUDE arglist PCLOSE'''
        p[0] = PubInclude (p[2], False, p.lineno (1), p.lineno (3))

    def p_load (self, p):
        '''load : PTLOAD arglist PCLOSE'''
        p[0] = PubInclude (p[2], True, p.lineno (1), p.lineno (3))

    def p_switch (self, p):
        '''switch : PTSWITCH LPAREN VAR RPAREN comma_opt switch_cases PCLOSE'''
        p[0] = PubSwitch (key = p[3], cases = p[6], start = p.lineno (1),
                          end = p.lineno (7))

    def p_comma_opt (self, p):
        '''comma_opt : 
                     | COMMA'''
        p[0] = None
        
    def p_switch_cases (self, p):
        '''switch_cases : switch_case
                        | switch_cases comma_opt switch_case'''
        if len (p) == 2: p[0] = [ p[1] ]
        else: 
            p[1].append (p[3])
            p[0] = p[1]

    def p_switch_case (self, p):
        '''switch_case : array'''
        p[0] = p[1]

    def p_arglist (self, p):
        '''arglist : LPAREN array_inner RPAREN'''
        p[0] = PubArgList ( p[2] )

    def p_array (self, p):
        '''array : array_open array_inner RPAREN'''
        p[0] = PubList ( p[2] )

    def p_array_open (self, p):
        '''array_open : LPAREN 
                      | ARRAY_OPEN'''
        p[0] = None

    def p_array_arg (self, p):
        '''array_arg : 
                     | arg'''
        if len (p) == 1: p[0] = None
        else: p[0] = p[1]

    def p_array_inner (self, p):
        '''array_inner : array_arg
                       | array_inner COMMA array_arg'''
        if len (p) == 2: 
            p[0] = [ p[1] ]
        else:
            p[1].append (p[3])
            p[0] = p[1]

    def p_arg (self, p):
        '''arg : string 
               | integer 
               | array 
               | dict 
               | null
               | range
               | env'''
        p[0] = p[1]

    def p_string (self, p):
        '''string : sstring 
                  | dstring'''
        p[0] = PubString (p[1])

    def p_sstring (self, p):
        '''sstring : SQUOTE string_frags SQUOTE'''
        p[0] = ''.join (p[2])

    def p_dstring (self, p):
        '''dstring : DQUOTE string_frags DQUOTE'''
        p[0] = ''.join (p[2])

    def p_string_frags (self, p):
        '''string_frags : 
                        | string_frags STRING'''
        if len (p) == 1: p[0] = []
        else:
            p[1].append (p[2])
            p[0] = p[1]

    def p_dict (self, p):
        '''dict : LBRACE bindings_opt RBRACE'''
        p[0] = p[2]

    def p_bindings_opt (self, p):
        '''bindings_opt :
                        | bindings'''
        if len (p) == 1: 
            p[0] = PubDict ()
        else:
            p[0] = p[1]

    def p_bindings (self, p):
        '''bindings : binding
                    | bindings COMMA binding'''
        if len (p) == 2: 
            p[0] = PubDict (p[1])
        else:
            p[1].add (p[3])
            p[0] = p[1]

    def p_binding (self, p):
        '''binding : bindkey equals arg'''
        p[0] = (p[1], p[3])

    def p_equals (self, p):
        '''equals : COLON 
                  | PERLARROW
                  | EQUALS'''
        p[0] = None

    def p_bindkey (self, p):
        '''bindkey : null 
                   | string 
                   | bind_var 
                   | integer'''
        p[0] = p[1]

    def p_bind_var (self, p):
        '''bind_var : VAR'''
        p[0] = PubString (p[1])

    def p_null (self, p):
        '''null : NULL'''
        p[0] = PubNull ()

    def p_integer (self, p):
        '''integer : INTEGER'''
        p[0] = PubInt (int (p[1]))

    def p_env (self, p):
        '''env : DLBRACE blocks DRBRACE'''
        p[0] = NestedHtml (HtmlBlock (p[2]))

    def p_range (self, p):
        '''range : RANGE'''
        p[0] = PubRange (p[1], p.lineno (1))

    def p_error(self, p):
        where = "EOF"
        if p: where = "'%s'" % p
        ret = "Syntax error at %s" % where
        raise Pub1Error, ret
    
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

class Runner:
    
    ##----------------------------------------

    def __init__ (self, debug = False):
        self._parser = Pub1Parser ()
        self._debug = debug
        self._regex = r'.*\.(html|conf|js)$'
        self._exclude = r'\.svn'
        self._verbose = False

    ##----------------------------------------

    def parse (self, data):
        '''Run a given file through the converter.'''
        if self._debug:
            lx = Pub1Lexer ()
            raw = '\n'.join ([str (x) for x in lx.tokenize (data)] )
            sys.stderr.write (raw)

        obj = self._parser.parse (data)
        oe = OutputEngine ()
        obj.output (oe)
        ret = None
        if oe.changes_made ():
            ret = str (oe)
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
        except Pub1Error, e:
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
    runner = Runner ()
    runner.config (argv)
    ok = runner.run ()
    rc = 0
    if not ok: rc = 5
    sys.exit (rc)

##=======================================================================

if __name__ == '__main__':
    main(sys.argv)

