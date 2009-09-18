/* -*-fundamental-*- */
/* $Id$ */

%{
#include "pub_parse.h"
#include "parse.h"
#include "qhash.h"
#define YY_STR_BUFLEN 20*1024

static void begin_P3_STR (char ch);
static bool end_P3_STR (char ch);
static void nlcount (int m = 0);
static void push_p3_env ();
static void pop_p3_func (void);

static void p3_regex_begin (char in);
static int  p3_regex_is_close_char (char c);
static void p3_regex_add (const char *in);
static int  p3_regex_bad_eof ();
static int  p3_regex_finish (const char *opts);
static void p3_regex_escape_sequence (const char *c);
static int  p3_identifier (const char *yyt);

static int bracket_check_eof (void);

int yy_ssln;
int yy_wss_nl;
int yywss;
int yyesc;
int yy_oldesc;
int yy_pt_com;  /* pass-throgh comment! */
char str_buf[YY_STR_BUFLEN];
int sbi;
char *eof_tok;
int yy_d_brace;
int yy_d_bracket;
vec<int> yy_d_bracket_linenos;
int yy_p3_depth;

static char yy_p3_regex_close_char;
static char yy_p3_regex_open_char;
static int yy_p3_regex_start_line;
strbuf yy_p3_regex_buf;
static char yy_p3_str_char;

static strbuf yy_json_strbuf;
static int    yy_json_str_line;

static int  json_str_end ();
static void json_str_begin (int mode);
static void json_str_addch (int ch);
static void json_str_add_unicode (const char *in);
static void json_str_addstr (const char *in);
static int  json_str_eof ();
void json_inc_lineno ();
static int yy_json_lineno = 1;
static int yy_json_mode = 0;
int json_error (str s);

%}

%option stack
%option noyywrap

P3IDENT [a-zA-Z_][a-zA-Z_0-9]*
WS	[ \t]
WSN	[ \t\n]
EOL	[ \t]*\n?

%x H TXLCOM TXLCOM3 
%x P3 P3_STR P3_REGEX 
%x C_COMMENT JSON JSON_STR JSON_SQ_STR

%%

<INITIAL>\n	{ PLINC; return ('\n'); }


<H>{
"{%"		{ push_p3_env (); return T_P3_OPEN; }

"%{"		{
		      yy_push_state (P3);
		      return T_P3_BEGIN_EXPR;
		}

\\+[%]"{"	|
\\+"{%"		|
\\"}}"          { yylval.str = yytext + 1; return T_HTML; }

"}}"		{ if (yy_d_brace > 0) {
		     yy_d_brace -- ;
		     yy_pop_state ();
                     return T_2R_BRACE; 
	          } else {
	 	     yylval.str = yytext; return T_HTML;
	          } 
                } 


[^%}{\\\[\]]+	{ yylval.str = yytext; nlcount (); return T_HTML; }
[%}\\{\[\]]	{ yylval.ch = yytext[0]; return T_CH; }

<<EOF>>		{  return bracket_check_eof(); }
}

<H,P3_STR>{
"[[[["		{
		   yy_d_bracket += 2;
		   bracket_mark_left (2);
		   yy_push_state (TXLCOM);
		}

"[[["		{
		   yy_d_bracket += 1;
		   bracket_mark_left (1);

		   if (yy_d_bracket == 1) {
		      yy_push_state (TXLCOM3);
		   } else {
		      yy_push_state (TXLCOM);
		   }
		}

"[["		{
		   yy_d_bracket ++;
		   bracket_mark_left (1);
		   if (yy_d_bracket > 1) 
		     yy_push_state (TXLCOM);
		}

"]]"		{ 
                  if (yy_d_bracket > 0) {
		     bracket_mark_right ();
		     yy_d_bracket--;
                  } else {
		     yylval.str = yytext;
		     return T_HTML;
		  }
                }

\\"["{2,4}	|
\\"]]"	        { yylval.str = yytext + 1; return T_HTML; }
}


<TXLCOM3>{
"]"{3}		{ 
		   yy_d_bracket --;
                   bracket_mark_right ();
		   yy_pop_state (); 
                }
\n		{ PLINC; }
"]"{1,2}        { /* ignore! */; }
[^\]\n]+	{ /* ignore */; }
}

<TXLCOM>{
"[["		{ yy_d_bracket++; bracket_mark_left (); }
\\"[""["+	{ /* ignore */ ; }

"["		|
"\\"		|
"]"		|
[^\]\[\\\n]+	{ /* ignore */ ; }

"]]"		{ 
		   yy_d_bracket--; 
		   bracket_mark_right();
		   if (yy_d_bracket <= 1) { yy_pop_state (); } 
                }

\n		{ PLINC; }
}


<H,TXLCOM,TXLCOM3>{
<<EOF>>		{  return bracket_check_eof(); }
}

<P3_STR>{
[%][{]		{ yy_push_state (P3); return T_P3_BEGIN_EXPR; }
\\n		{ yylval.ch = '\n'; return T_P3_CHAR; }
\\t		{ yylval.ch = '\t'; return T_P3_CHAR; }
\\r		{ yylval.ch = '\r'; return T_P3_CHAR; }
\n		{ PLINC; yylval.ch = yytext[0]; return T_P3_CHAR; }
\\.		{ yylval.ch = yytext[1]; return T_P3_CHAR; }
[%]		{ yylval.ch = yytext[0]; return T_P3_CHAR; }
[\[\]]		{ yylval.ch = yytext[0]; return T_P3_CHAR; }
["']		{ 
                   if (end_P3_STR (yytext[0])) {
		      return yytext[0];
		   } else {
		      yylval.ch = yytext[0];
		      return T_P3_CHAR;
		   }
                }

[^\\%"'\n\[\]]+	{ yylval.str = yytext; return T_P3_STRING; }

.		{ return yyerror ("illegal token found in string"); }

<<EOF>>         {   
		    bracket_check_eof ();
 		    return yyerror (strbuf ("EOF found in str started on "
		  	 	 	  "line %d", yy_ssln)); 
                }
}

<P3>{

\n		{ PLINC; }

[Tt]rue		{ return T_P3_TRUE; }
[Ff]alse	{ return T_P3_FALSE; }
{P3IDENT}	{ return p3_identifier (yytext); }
r[#/!@%{<([]	{ p3_regex_begin (yytext[1]); }


([0-9]+|0x[0-9a-f]+) { yylval.str = yytext; return T_P3_UINT; }
-?[0-9]*\.[0-9]+     { yylval.str = yytext; return T_P3_FLOAT; }
-[0-9]+              { yylval.str = yytext; return T_P3_INT; }

==		 { return T_P3_EQEQ; }
!=		 { return T_P3_NEQ; }
[<]=		 { return T_P3_LTEQ; }
>=		 { return T_P3_GTEQ; }
=>		 { return yytext[0]; }
[%()!=><,[\].+*:;/-] { return yytext[0]; }
[{]		 { yy_push_state (P3); return yytext[0]; }
[}]		 { yy_pop_state (); return yytext[0]; }
"||"		 { return T_P3_OR; }
"|"		 { return T_P3_PIPE; }
&&		 { return T_P3_AND; }
"%}"		 { pop_p3_func (); return T_P3_CLOSE; }

[ \t\r]+	 { /* ignore */ }
["'] 		 { begin_P3_STR(yytext[0]); return yytext[0]; }

[/][/].*$        { /* comment -- strip out */ }
[/][*]           { yy_push_state (C_COMMENT); }

"{{"		{ 
   	     	   yy_d_brace ++; 
		   yy_push_state (yywss ? WH : H);
	 	   return T_2L_BRACE; 
		}
.		{ return yyerror ("illegal token in Pub v3 environment"); }
}

<P3_REGEX>{
\n			{ PLINC; p3_regex_add (yytext); }
[#/!@%}>)\]][a-zA-Z]*	{ 
			  if (p3_regex_is_close_char (yytext[0])) {
			     return p3_regex_finish (yytext + 1);
			  } else {  
			     p3_regex_add (yytext);
			  }
                        }

\\[#/!@%}>)\]]		{ p3_regex_escape_sequence (yytext); }

[^#/!@%}>)\]\n\\]+	{ p3_regex_add (yytext); }

<<EOF>>			{
			   return p3_regex_bad_eof ();
			}
}

<JSON>{
\n		     { json_inc_lineno (); }
[ \t\r]+	     { /* ignore */ }
[[{:,}\]]	     { return yytext[0]; }
([0-9]+|0x[0-9a-f]+) { yylval.str = yytext; return T_P3_UINT; }
-?[0-9]*\.[0-9]+     { yylval.str = yytext; return T_P3_FLOAT; }
-[0-9]+              { yylval.str = yytext; return T_P3_INT; }
true		     { return T_P3_TRUE; }
false		     { return T_P3_FALSE; }
null		     { return T_P3_NULL; }
["]		     { json_str_begin (JSON_STR); }
[']		     { json_str_begin (JSON_SQ_STR); }
.		     { return json_error ("illegal token in JSON environment");}
}

<JSON_STR>{
["]		     { return json_str_end (); }
\\\"		     { json_str_addch ('\"'); }
[^\\\n"]+	     { json_str_addstr (yytext); }
}

<JSON_SQ_STR>{
[']		     { return json_str_end (); }
\\\'		     { json_str_addch ('\"'); }
[^\\\n']+	     { json_str_addstr (yytext); }
}

<JSON_STR,JSON_SQ_STR>{
\n		     { json_inc_lineno (); json_str_addch (yytext[0]); }
<<EOF>>		     { return json_str_eof (); }
\\\\		     { json_str_addch ('\\'); }
\\n		     { json_str_addch ('\n'); }
\\r		     { json_str_addch ('\r'); }
\\t		     { json_str_addch ('\t'); }
\\u[a-fA-F0-9]{4}    { json_str_add_unicode (yytext + 2); }
\\b		     { json_str_addch ('\b'); }
\\[/]		     { json_str_addch ('/'); }
\\.                  { 
		         if (ok_pub3_json_strict_escaping) {
		            strbuf b ("illegal escape sequence in "
			              "string ('%s')", yytext);
		            return json_error (b);
			 } else {
			    json_str_addch (yytext[1]);
			 }
                     }
}

<C_COMMENT>{
\n		{ PLINC; }
"*/"		{ yy_pop_state (); }
[^*\n]+		{ /* ignore */ }
}


%%

void
nlcount (int m)
{
  int n = 0;
  for (char *y = yytext; *y; y++)
    if (*y == '\n') {
      n++;
      if (m && m == n) 
        break;
    }
  PFILE->inc_lineno (n);
}

int
yyerror (str msg)
{
  if (yy_json_mode) {
    json_error (msg);
  } else {
    if (!msg) 
      msg = "bailing out due to earlier warnings";
    PWARN(msg);
    PARSEFAIL;	
    yyterminate ();
  }
  return 0;
}

void
yy_parse_fail()
{
  if (!yy_json_mode) {
    PARSEFAIL;
  }
}

int
yywarn (str msg)
{
  PWARN("lexer warning: " << msg);
  return 0;
}

void
yy_push_pubstate (pfile_type_t t)
{
  switch (t) {
  case PFILE_TYPE_CONF:
    yy_push_state (H);
    break;
  case PFILE_TYPE_H:
    yy_push_state (H);
    break;
  default:
    fatal << "unknown lexer state\n";
  }
}

void
yy_pop_pubstate ()
{
  yy_pop_state ();
}

void
yyswitch (yy_buffer_state *s)
{
  yy_switch_to_buffer (s);
}

yy_buffer_state *
yycreatebuf (FILE *fp)
{
  return (yy_create_buffer (fp, YY_BUF_SIZE));
}

void
gcc_hack_use_static_functions ()
{
  assert (false);
  char buf[2];
  yyunput (yy_top_state (), buf);
}

void
scanner_reset (void)
{
   yy_d_brace = 0;
   yy_d_bracket = 0;
   yy_d_bracket_linenos.clear ();
}


int
bracket_check_eof (void)
{
  if (yy_d_bracket > 0) {
    yyerror (strbuf ("Unbalanced brackets at EOF; started at line %d",
       unbalanced_bracket ()));
  }
  return 0;
}

void
push_p3_env ()
{
   yy_p3_depth++;
   yy_push_state (P3);
}

void
pop_p3_func (void)
{
  if (yy_p3_depth <= 0) {
    yyerror ("Unbalanced '{%' Pub3 tag at EOF\n");
  } else {
    yy_p3_depth --;
    yy_pop_state ();
  }
}

//-----------------------------------------------------------------------

// P3 perl-style regex's!

void 
p3_regex_begin (char ch) 
{
  yy_p3_regex_start_line = PLINENO;
  char open = ch, close = '\0';

  switch (ch) {
  case '#':
  case '!':
  case '@':
  case '/':
  case '%':
    close = ch;
    break;
  case '{': close = '}'; break;
  case '<': close = '>'; break;
  case '(': close = ')'; break;
  case '[': close = ']'; break;
  default:
    yyerror (strbuf ("unexpected P3 regex delimiter: '%c'\n", ch));
    break;
  }

  yy_p3_regex_close_char = close;
  yy_p3_regex_open_char = open;
  yy_push_state (P3_REGEX);
}

int  
p3_regex_is_close_char (char c) 
{
  return c == yy_p3_regex_close_char;
}

void 
p3_regex_add (const char *in) 
{
   yy_p3_regex_buf.cat (in, true);
}

void
p3_regex_escape_sequence (const char *in)
{
  // if the char being escape is exactly the close sequence, or
  // the escape character, then strip off the escape!
  if (in[1] == yy_p3_regex_close_char || in[1] == '\\') { in++; }
  p3_regex_add (in);
}

int  
p3_regex_bad_eof () 
{
  yyerror (strbuf ("Found EOF when looking for end of regex, "
           "started on line %d\n", yy_p3_regex_start_line));
  return -1;
}

int
p3_regex_finish (const char *opts) 
{
  yylval.regex.regex = yy_p3_regex_buf;
  yylval.regex.opts = opts;
  yy_pop_state ();
  yy_p3_regex_buf.tosuio ()->clear ();
  return T_P3_REGEX;
}

class p3_identifier_tab_t {
public:
  p3_identifier_tab_t () {
    _tab.insert ("for", T_P3_FOR);
    _tab.insert ("if", T_P3_IF);
    _tab.insert ("locals", T_P3_LOCALS);
    _tab.insert ("include", T_P3_INCLUDE);
    _tab.insert ("universals", T_P3_UNIVERSALS);
    _tab.insert ("print", T_P3_PRINT);
    _tab.insert ("elif", T_P3_ELIF);
    _tab.insert ("else", T_P3_ELSE);
    _tab.insert ("empty", T_P3_EMPTY);
    _tab.insert ("load", T_P3_LOAD);
    _tab.insert ("print", T_P3_PRINT);
    _tab.insert ("case", T_P3_CASE);
    _tab.insert ("switch", T_P3_SWITCH);
    _tab.insert ("deafult", T_P3_DEFAULT);
  }

  bool lookup (const char *in, int *out) {
    bool ret = false;
    int *outp = _tab[in];
    if (outp) { ret = true; *out = *outp; }
    return ret;
  }
private:
  qhash<const char *, int> _tab;
};

static p3_identifier_tab_t p3_id_tab;

int
p3_identifier (const char *yyt)
{
   int ret;
   if (!p3_id_tab.lookup (yyt, &ret)) {
     yylval.str = yyt;
     ret = T_P3_IDENTIFIER;
   }
   return ret;
}

//-----------------------------------------------------------------------

int
yy_get_json_lineno ()
{
  return yy_json_lineno;
}

void
json_str_begin (int str_state)
{
  yy_json_str_line = yy_json_lineno;
  yy_json_strbuf.tosuio ()->clear ();
  yy_push_state (str_state);
}

void
json_str_addch (int ch)
{
  char buf[2];
  buf[1] = '\0';
  buf[0] = ch;
  yy_json_strbuf.cat (buf, true);
}

void
json_str_addstr (const char *x)
{
  yy_json_strbuf.cat (x, true);
}

void 
json_str_add_unicode (const char *in)
{
   char *ep;
   long l = strtol (in, &ep, 16);
   if (*ep == '\0' && l > 0 && l <= 0xff) {
      json_str_addch (l);
   }
}

int
json_str_eof ()
{
  strbuf e ("EOF on JSON string started on line %d", yy_json_str_line);
  return json_error (e);
}

int
json_str_end ()
{
  yylval.str = str (yy_json_strbuf);
  yy_json_strbuf.tosuio ()->clear ();
  yy_pop_state ();
  return T_P3_STRING;
}

void
yy_parse_json (str s)
{
  yy_json_lineno = 1;
  yy_json_mode = 1;
  yy_push_state (JSON);
  yy_scan_bytes (s.cstr (), s.len());
}

void
json_inc_lineno ()
{
  yy_json_lineno ++;
}

int
json_error (str s)
{
  warn << "<json-input>:" << yy_json_lineno << ": " << s << "\n";
  yyterminate ();
  return 0;
}

//-----------------------------------------------------------------------


/*
// States:
//   H - HTML w/ includes and variables and switches and such
//   TXLCOM - Translator comment
//   TXLCOM3 - Translator comment state 3
//   P3 -- Pub v3 (expanded boolean logic)
//   P3_STR -- Pub v3 string (with shell-like expansions)
//   P3_REGEX -- For parsing p3-style regex's
//   C_COMMENT - style C comments
//   JSON JSON_STR JSON_SQ_STR - For json
//
*/
