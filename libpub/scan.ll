/* -*-fundamental-*- */
/* $Id$ */

%{
#include "pub_parse.h"
#include "parse.h"
#define YY_STR_BUFLEN 20*1024

static void begin_PSTR (int i, int mode);
static void end_PSTR ();
static void begin_STR (int i, int j);
static int  end_STR ();
static int addch (int c1, int c2);
static int addstr (const char *c, int l);
static void nlcount (int m = 0);

static void bracket_mark_left (int n = 1);
static void bracket_mark_right (void);
static int unbalanced_bracket (void);
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

%}

%option stack
%option noyywrap

VAR	[a-zA-Z_][a-zA-Z_0-9]*
HNAM	[a-zA-Z_][a-zA-Z_0-9-]*
HVAL	[^ \t\n\r"'>=$]*[^ \t\n\r/"'>=$]
ST	[Ss][Cc][Rr][Ii][Pp][Tt]
PRET    [Pp][Rr][Ee]
WS	[ \t]
WSN	[ \t\n]
EOL	[ \t]*\n?
TPRFX	"<!--#"[ \t]*
TCLOSE	[ \t]*[;]?[ \t]*"-->"

%x STR SSTR H HTAG PTAG PSTR PVAR WH HCOM JS 
%x PRE PSTR_SQ TXLCOM TXLCOM3

%%

<INITIAL>\n	{ PLINC; return ('\n'); }

<PTAG>{
"-->"		{ yy_pop_state (); return T_EPTAG; }
}

<PTAG>{
{WS}+		/* discard */ ;
\n		{ PLINC; }

"{{"		{ 
   	     	   yy_d_brace ++; 
		   yy_push_state (yywss ? WH : H);
	 	   return T_2L_BRACE; 
		}

=>		|
[(),{}=;]	return yytext[0];


int(32(_t)?)?[(]	return T_INT_ARR; 
char[(]			return T_CHAR_ARR;
int64(_t)?[(]		return T_INT64_ARR;
int16(_t)?[(]		return T_INT16_ARR;

u_int(32(_t)?)?[(]	return T_UINT_ARR;
u_int16(_t)?[(]		return T_UINT16_ARR;


{VAR}		{ yylval.str = yytext; return T_VAR; }

[+-]?[0-9]+	|
[+-]?0x[0-9]+	{ yylval.str = yytext; return T_NUM; }

\"		{ begin_PSTR (1, PSTR); return (yytext[0]); }

"//".*$		/* discard */ ;

.		{ return yyerror ("illegal token found in GUY/PTAG "
	                          "environment"); }
}

<H>\n			{ PLINC; return (yytext[0]); }

<H,WH>{
{TPRFX}include		{ yy_push_state (PTAG); return T_PTINCLUDE; }
{TPRFX}load		{ yy_push_state (PTAG); return T_PTLOAD; }
{TPRFX}setl		{ yy_push_state (PTAG); return T_PTSETL; }
{TPRFX}set		{ yy_push_state (PTAG); return T_PTSET; }
{TPRFX}switch		{ yy_push_state (PTAG); return T_PTSWITCH; }
{TPRFX}"#"	    	|
{TPRFX}com(ment)?	{ yy_pt_com = 0; yy_push_state (HCOM); }
{TPRFX}[Rr][Ee][Mm]	{ 
			  yy_pt_com = 1; 
 			  begin_STR (HCOM, 0);
			  addstr ("<!--", 4);
	  		}
}

<TXLCOM3>{
"]"{3}		{ 
		   yy_d_bracket --;
                   bracket_mark_right ();
		   yy_pop_state (); 
                }
\n		{ PLINC; }
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


<H,WH,JS,PSTR,PTAG,HTAG,PSTR_SQ>{
"${"		{ yy_push_state (PVAR); return T_BVAR; }

"[[[["		{
		   yy_d_bracket += 2;
		   bracket_mark_left (2);
		   yy_push_state (TXLCOM);
		}

"[[["		{
		   yy_d_bracket += 1;
		   bracket_mark_left (1);

		   // [[ ... [[[     actually goes to regular comment mode
		   // [[[ ... [[     goes to "trips" mode
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

\\+[$]"{" 	|
\\"}}"		|
\\"["{2,4}	|
\\"]]"	        { yylval.str = yytext + 1; return T_HTML; }

"}}"		{ if (yy_d_brace > 0) {
		     yy_d_brace -- ;
		     yy_pop_state ();
                     return T_2R_BRACE; 
	          } else {
	 	     yylval.str = yytext; return T_HTML;
	          } 
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

[$}\[\]]	{ yylval.ch = yytext[0]; return T_CH; }
}
<H,WH,JS,PTAG,HTAG,TXLCOM,TXLCOM3>{
<<EOF>>		{  return bracket_check_eof(); }
}

<H>{
[^$}\\<\[\]]+	{ yylval.str = yytext; nlcount (); return T_HTML; }
"<"		{ yylval.ch = yytext[0]; return T_CH; }
}

<H>{
\\		{ yylval.ch = yytext[0]; return T_CH; }
}


<WH>{	
{WSN}+		{ nlcount (); return (' '); }
"<!"		{ yylval.str = yytext; return T_HTML; }
[<][/?%]?	{ yy_push_state (HTAG); yylval.str = yytext; return T_BTAG; }
\<{ST}/[ \t\n>]	{ yy_push_state (JS); yy_push_state (HTAG); 
	          yylval.str = yytext; return T_BJST; }

{TPRFX}{ST}{TCLOSE} { yy_push_state (JS); return T_BJS_SILENT; }

\<!--		{ yy_pt_com = 0; yy_push_state (HCOM); }


\<{PRET}{WSN}*\> { yy_push_state (PRE); nlcount (); yylval.str = yytext; 
	          return T_BPRE; }
}

<PRE>{
[^<]+		{ yylval.str = yytext; nlcount (); return T_HTML; }
"</"{PRET}\>	{ yy_pop_state (); yylval.str = yytext; return T_EPRE; }
\<		{ yylval.ch = yytext[0]; return T_CH; }
}

<JS>{
"</"{ST}{WS}*\>	{ yy_pop_state (); yylval.str = yytext; return T_EJS; }
{TPRFX}"/"{ST}{TCLOSE} { yy_pop_state (); return T_EJS_SILENT; }
[}$@<\\]	{ yylval.ch = yytext[0]; return T_CH; }
[^\\$@<}]+	{ yylval.str = yytext; nlcount (); return T_HTML; }
}

<HCOM>{
\n		{ PLINC; if (yy_pt_com) { addch (yytext[0], -1); } }
--\>		{ 
		   if (yy_pt_com) {
			addstr (yytext, yyleng);
			end_STR (); /* calls yy_pop_state (); */
			return T_HTML;
		   } else {
			yy_pop_state ();
		   }
		}

[^-\n]*		{ if (yy_pt_com) { addstr (yytext, yyleng); } }
-		{ if (yy_pt_com) { addch (yytext[0], -1); } }
}

<WH>{
[^$@\\<\n\t}\[\] ]+	{ yylval.str = yytext; return T_HTML; }
\\		 	{ yylval.ch = yytext[0]; return T_CH; }
}

<HTAG>{
\n		{ PLINC; }
["]		{ begin_PSTR (0, PSTR); return (yytext[0]); }
[']		{ begin_PSTR (0, PSTR_SQ); return (yytext[0]); }

[%?/]?">" 	{ yy_pop_state (); yylval.str = yytext; return T_ETAG; }

{WS}+		/* discard */;
{HNAM}		{ yylval.str = yytext; return T_HNAM; }
{HVAL}		{ yylval.str = yytext; return T_HVAL; }
=		{ return (yytext[0]); }
.		{ return yyerror ("illegal token found in parsed HTAG"); }
}

<SSTR,STR>\n	{ PLINC; addch ('\n', -1); }
<STR>\" 	{ return (end_STR ()); }
<SSTR>\'	{ return (end_STR ()); }

<STR,SSTR>{
\\n  		addch ('\n', 'n');
\\t  		addch ('\t', 't');
\\r		addch ('\r', 'r');
\\b		addch ('\b', 'b');
\\f		addch ('\f', 'f');
\\(.|\n)	addch (yytext[1], yytext[1]);
}

<STR>[^\\\n\"]+		addstr (yytext, yyleng);
<SSTR>[^\\\n\']+	addstr (yytext, yyleng);

<PSTR>{
\n		{ return yyerror ("unterminated parsed string"); }
\\[\\"'tn]	{ if (yyesc) { yylval.ch = yytext[1]; return T_CH; }
	  	  else { yylval.str = yytext; return T_STR; } }
\\.		{ return yyerror ("illegal escape sequence"); }
\"		{ end_PSTR (); return (yytext[0]); }
[^"\\$@%}\[\]]+	{ yylval.str = yytext; return T_STR; }
}

<PSTR_SQ>{
\n		{ return yyerror ("unterminated parsed string"); }
\\[\\'"tn]	{ if (yyesc) { yylval.ch = yytext[1]; return T_CH; }
	  	  else { yylval.str = yytext; return T_STR; } }
\\.		{ return yyerror ("illegal escape sequence"); }
\'		{ end_PSTR (); return (yytext[0]); }
[^'\\$@%}\[\]]+	{ yylval.str = yytext; return T_STR; }
}


<STR,PSTR,SSTR,PSTR_SQ>{
<<EOF>>		{ 
		  return yyerror (strbuf ("EOF found in str started on "
                                          "line %d", yy_ssln)); 
		}
}

<PVAR>{
{VAR}		{ yylval.str = yytext; return T_VAR; }
\}		{ yy_pop_state (); return (yytext[0]); }
.		{ return yyerror ("illegal token found in ${..}"); }
}

.		{ return yyerror ("illegal token found in input"); }

%%

void
begin_PSTR (int i, int state)
{
  yy_oldesc = yyesc;
  yyesc = i;
  yy_push_state (state);
  yy_ssln = PLINENO;
}

void
end_PSTR ()
{
  yyesc = yy_oldesc;
  yy_pop_state ();
}

void
begin_STR (int s, int e)
{
  sbi = 0;
  yy_oldesc = yyesc;
  yyesc = e;
  yy_push_state (s);
  yy_ssln = PLINENO;
}


int
end_STR ()
{
  str_buf[sbi] = '\0';
  yylval.str = str_buf;
  yyesc = yy_oldesc;
  yy_pop_state ();
  return T_STR;
}

int
addch (int c1, int c2)
{
  int len = (yyesc || c2 < 0) ? 1 : 2;
  if (sbi >= YY_STR_BUFLEN - len)
    return yyerror ("string buffer overflow");
  if (yyesc || c2 < 0)
    str_buf[sbi++] = c1;
  else
    sbi += sprintf (str_buf + sbi, "\\%c", c2);
  return 1;
}

int
addstr (const char *s, int l)
{
  if (sbi + l >= YY_STR_BUFLEN - 1)
    return yyerror ("string buffer overflow");
  memcpy (str_buf + sbi, s, l);
  sbi += l;
  return 1;
}

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
  if (!msg) 
    msg = "bailing out due to earlier warnings";
  PWARN(msg);
  PARSEFAIL;	
  yyterminate ();
  return 0;
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
  case PFILE_TYPE_WH:
    yy_push_state (WH);
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

void
bracket_mark_left (int l)
{
   for (int i = 0; i < l; i++) {
     yy_d_bracket_linenos.push_back (PLINENO);
   }
}

void
bracket_mark_right (void)
{
   if (yy_d_bracket_linenos.size ())
     yy_d_bracket_linenos.pop_back ();
}

int
unbalanced_bracket (void)
{
  int ret = 0;
  if (yy_d_bracket_linenos.size ())
    ret = yy_d_bracket_linenos.back ();
  return ret;
}

int
bracket_check_eof (void)
{
  if (yy_d_bracket > 0) {
    yyerror (strbuf ("Unbalanced [[ or [[[ at EOF; started at line %d",
       unbalanced_bracket ()));
  }
  return 0;
}


/*
// States:
//   STR - string within an HTML tag or within regular mode
//   SSTR - string with single quotes around it
//   H - HTML w/ includes and variables and switches and such
//   HTAG - Regular tag within HTML mode
//   PTAG - Pub tag within HTML
//   PSTR - Parsed string
//   PVAR - Variable state (within ${...})
//   WH - White-space-stripped HTML
//   HCOM - HTML Comment
//   JS - JavaScript
//   TXLCOM - Translator comment
//   TXLCOM3 - Translator comment state 3
//
*/
