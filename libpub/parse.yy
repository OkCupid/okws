/* -*-fundamental-*- */
/* $Id$ */

%{
#include "pub.h"
#include "pub_parse.h"
#include "parr.h"

%}

%token <str> T_NUM
%token <str> T_HNAM
%token <str> T_HVAL
%token <str> T_CODE
%token <str> T_STR
%token <str> T_VAR
%token <ch>  T_CH
%token <str> T_ETAG
%token <str> T_BJST
%token <str> T_EJS
%token <str> T_HTML
%token <str> T_GCODE
%token <str> T_BTAG
%token <str> T_BPRE
%token <str> T_EPRE

%token T_BGUY
%token T_EGUY
%token T_BWGH
%token T_BGH
%token T_EGH
%token T_VARS
%token T_UVARS
%token T_PRINT
%token T_CTINCLUDE
%token T_INCLUDE
%token T_PTINCLUDE
%token T_PTINCLIST
%token T_PTSET
%token T_PTSWITCH
%token T_EPTAG
%token T_BVAR
%token T_ETAG
%token T_EC_EC
%token T_EC_G
%token T_EC_M
%token T_EC_EM
%token T_EC_C
%token T_EC_CF
%token T_EC_V
%token T_EC_UV
%token T_EC_CLOSE
%token T_BCONF
%token T_BGCODE
%token T_BGCCE
%token T_INIT_PDL

%token T_INT_ARR
%token T_UINT_ARR
%token T_CHAR_ARR
%token T_INT16_ARR
%token T_UINT16_ARR
%token T_INT64_ARR
%token T_UINT64_ARR

%type <str> var str1 bname 
%type <num> number
%type <pvar> pvar evar gcode
%type <pval> bvalue arr i_arr g_arr
%type <sec> guy_print htag htag_list javascript pre b_js_tag
%type <el> ptag guy_vars guy_func
%type <func> guy_funcname
%type <pstr> pstr
%type <arg> arg aarr
%type <parr> i_arr_open

%%
file: gfile       {}
	| hfile   {}
	| ecfile  {}
	| conffile {}
	;

conffile: T_BCONF aarr {}
	;

hfile: html 
	;

ecfile: begin_ec ec_vars_star ec_csecs ec_main ec_vars_star ec_csecs {}
	;

begin_ec: T_EC_EC '(' var ',' var ')' T_EC_CLOSE
	{
	  PFILE->add_section (New pfile_ec_header_t (PLINENO, $3, $5));
	}
	;

ec_csec_f: T_EC_CF
	{
	  PFILE->push_section (New pfile_code_t (PLINENO));
	}
	ec_code T_EC_CLOSE
	{
	  PSECTION->add ('\n');
	  PFILE->add_section2 ();
	}
	;

ec_csec_h: T_EC_C
	{
	  PFILE->push_section (New pfile_code_t (PLINENO));
	}
	ec_code T_EC_CLOSE 
	{
	  PSECTION->add ('\n');
	  PFILE->add_section2 ();
	  PFILE->push_section (New pfile_html_sec_t (PLINENO));
	}
	ec_html 
	{
	  PFILE->add_section2 ();
	}
	;

ec_html: /* empty */
	| ec_html ec_html_frag
	;

ec_html_frag: ec_gsec {}
	| ec_vars {}
	| html_part {}
	;

ec_vars_star: /* empty */
	| ec_vars_star ec_vars
	;

ec_main: T_EC_M 
	{ 
	  PFILE->push_section (New pfile_ec_main_t (PLINENO));
	  PFILE->push_section (New pfile_html_sec_t (PLINENO));	
	} 
	ec_html 
	{
	  PFILE->add_section2 ();
	}
	ec_csecs T_EC_EM 
	{
	  PFILE->add_section ();
	}
	;

ec_csecs: /* empty */
	| ec_csecs ec_csec
	;

ec_csec: ec_csec_f {}
	| ec_csec_h {}
	;

ec_gsec: T_EC_G
	{
	  PFILE->push_section (New pfile_ec_gs_t (PLINENO));
	}
	guy_code T_EC_CLOSE
	{
	  PFILE->add_section2 ();
	}
	;

ec_vars: ec_vars_mode varlist ';' T_EC_CLOSE
	{
	  PFILE->push_section (New pfile_ec_gs_t (PLINENO));
	  PSECTION->add (New pfile_gframe_t (PGVARS));
	  PFILE->add_section2 ();
	  PGVARS = NULL;
	}
	;

ec_vars_mode: T_EC_V	{ PGVARS = New gvars_t (); }
	| T_EC_UV	{ PGVARS = New guvars_t (); }
	;

ec_code: code_frag
	| ec_code code_frag
	;

code_frag: T_CODE 	{ PSECTION->add ($1); }
	| T_CH		{ PSECTION->add ($1); }
	;

gfile: gfile_section
	| gfile gfile_section
	;

gfile_section: T_CODE	    { PSECTION->add ($1); }
	| '/'		    { PSECTION->add ('/'); }
	| guy_section 	    {}
	;

guy_section: T_BGUY
	{
	  PFILE->add_section ();
	  PFILE->push_section (New pfile_gs_t (PLINENO));
	}
	guy_code T_EGUY
	{
	  PFILE->add_section ();
	  PFILE->push_section (New pfile_code_t (PLINENO));
	}
	;	

guy_code: /* empty */ {}
	| guy_code guy_command
	;

guy_command: guy_vars { PSECTION->add ($1); }
	| guy_func    { PSECTION->add ($1); }
	| guy_print   { PSECTION->add ($1, false); }
	;

guy_vars: guy_vars_mode varlist ';'
	{
	  $$ = New pfile_gframe_t (PGVARS);
	  PGVARS = NULL;
	}
	;

guy_vars_mode: T_VARS  { PGVARS = New gvars_t (); }
	| T_UVARS      { PGVARS = New guvars_t (); }
	;

varlist: var 
	{
	  PGVARS->add ($1);
	}
	| varlist ',' var 
	{
	  PGVARS->add ($3);
	}
	;

guy_print: T_PRINT '(' var ')' 
	{
          pfile_gprint_t *gp = New pfile_gprint_t (PLINENO, $3);
	  PFILE->push_section (gp);
	}
	ghtml_block
	{
          $$ = PFILE->pop_section ();
	}
	;

ghtml_block: T_BGH html T_EGH
	| T_BWGH html T_EGH
	;

html: /* empty */ {}
	| html html_part
	;

html_part: T_HTML 	{ PSECTION->hadd ($1); }
	| '\n'		{ PSECTION->hadd ('\n'); }	
	| evar		{ PSECTION->hadd (New pfile_var_t ($1, PLINENO)); }
	| T_CH		{ PSECTION->hadd ($1); }
	| htag		{ PSECTION->hadd ($1); PLASTHTAG = PHTAG; }
	| ' ' 		{ PSECTION->hadd_space (); }
	| javascript	{ PSECTION->hadd ($1); } 
	| ptag		{ PSECTION->hadd ($1); }
	| pre		{ PSECTION->hadd ($1); }
	;

evar:     pvar
	| gcode
	;
	
ptag: ptag_func ptag_list ptag_close 
	{
	  if (PFUNC->validate ())
	    PFUNC->explore (EXPLORE_PARSE);
	  else
	    PARSEFAIL;
	  $$ = PFUNC;
	  PFUNC = NULL;
	}
	;

ptag_close: ';' T_EPTAG
	| T_EPTAG
	;

ptag_func: T_PTINCLUDE	{ PFUNC = New pfile_include_t (PLINENO); }
	| T_PTSET	{ PFUNC = New pfile_set_func_t (PLINENO); }
	| T_PTSWITCH	{ PFUNC = New pfile_switch_t (PLINENO); }
	| T_PTINCLIST	{ PFUNC = New pfile_inclist_t (PLINENO); }
	;

javascript: b_js_tag js_code T_EJS 
	{
	  PSECTION->add ($3);
	  $$ = PSECTION;
	  PFILE->pop_section ();
	}
	;

pre: 	T_BPRE
	{
	  PFILE->push_section (New pfile_html_sec_t (PLINENO));
	  PSECTION->hadd ((PHTAG = New pfile_html_tag_t (PLINENO, $1)));
	}
	pre_body T_EPRE
	{
	  PSECTION->hadd ((PLASTHTAG = New pfile_html_tag_t (PLINENO, $4)));
	  $$ = PSECTION;
	  PFILE->pop_section ();
	}
	;

pre_body: /* empty */
	| pre_body pre_part
	;

pre_part: T_CH		{ PSECTION->add ($1); }
	| T_HTML	{ PSECTION->add ($1); }
	;
		
b_js_tag: T_BJST 
	{ 
 	  /* as a hack, we won't treat JavaScript section as explicit tags */
 	  PFILE->push_section (New pfile_html_sec_t (PLINENO));
	  PSECTION->add ($1);
  	  /* PSECTION->htag_space (); */
	} 
	htag_list T_ETAG
	{  
 	  PSECTION->add ($4);
	}
	;

js_code: /* empty */ 
	| js_code js_code_elem
	;

js_code_elem: T_CH  { PSECTION->add ($1); }
	| T_HTML    { PSECTION->add ($1); }
	| evar      { PSECTION->add (New pfile_var_t ($1, PLINENO)); }
	;

htag: T_BTAG
	{
	  PFILE->push_section ((PHTAG = New pfile_html_tag_t (PLINENO, $1)));
	}
  	htag_name htag_list T_ETAG
	{
	  PSECTION->add ($5);
	  $$ = PSECTION;
	  PFILE->pop_section ();
	}
	;

htag_list: /* empty */ {}
	| htag_list htag_elem
	;

htag_elem: htag_name '=' { PSECTION->add ('='); } htag_val
	| htag_name
	;

htag_name: T_HNAM  
	{ 
	   PSECTION->htag_space ();
           PSECTION->add ($1); 
        }
	| evar     
	{ 
	   PSECTION->htag_space ();
	   PSECTION->add (New pfile_var_t ($1, PLINENO));  
	}
	;

htag_val: T_HNAM   { PSECTION->add ($1); }
	| T_HVAL   { PSECTION->add ($1); }
	| evar     { PSECTION->add (New pfile_var_t ($1, PLINENO)); }
	| pstr     
	{ 
 	   PSECTION->add ('"');
	   PSECTION->add (New pfile_pstr_t ($1)); 
 	   PSECTION->add ('"');
	}
	| T_STR
	{
	   PSECTION->add ('\'');
	   PSECTION->add ($1); 
	   PSECTION->add ('\'');
	}
	;

guy_func: guy_funcname 
	{
	  ARGLIST = New refcounted<arglist_t> ();
	}
	'(' arglist ')' ';'
	{
	  if (!$1->add (ARGLIST)) {
	    PARSEFAIL;
          } else {
	    $1->explore (EXPLORE_PARSE);
	    if (!$1->validate ())
	      PARSEFAIL;
          }
	  ARGLIST = NULL;
	  $$ = $1;
	}
	;

guy_funcname: T_CTINCLUDE  { $$ = New pfile_g_ctinclude_t (PLINENO); }
	| T_INCLUDE        { $$ = New pfile_g_include_t (PLINENO); }
	| T_INIT_PDL	   { $$ = New pfile_g_init_pdl_t (PLINENO, PFILE); }
	;

parg:   '('
	{
	  ARGLIST = New refcounted<arglist_t> ();
	} 
	arglist ')'
	{
	  if (!PFUNC->add (ARGLIST))
	    PARSEFAIL;	    
	  ARGLIST = NULL;
	}
	;

ptag_list: parg
	| ptag_list ',' parg
	;

arglist: arg 		    { ARGLIST->push_back ($1); }
	| arglist ',' arg   { ARGLIST->push_back ($3); }
	;

arg: /* empty */  { $$ = New refcounted<pval_null_t> (); }
	| var     { $$ = New refcounted<pstr_t> ($1); }
	| pstr    { $$ = $1; }
	| number  { $$ = New refcounted<pint_t> ($1); }
	| aarr    { $$ = $1; }
	| pvar    { $$ = New refcounted<pstr_t> ($1); }
	;

aarr: '{' 
	{
	  PAARR = New refcounted<aarr_arg_t> ();
	} 
	bind_list '}'
	{
	  $$ = PAARR;
	  PAARR = NULL;
	}
	; 

bind_list: binding
	| bind_list ',' binding
	;

binding: bname '=' bvalue	
	{
	  PAARR->add (New nvpair_t ($1, $3));
	}
	;

bname: var
	| str1
	;

bvalue:   number   { $$ = New refcounted<pint_t> ($1); }
	| var      { $$ = New refcounted<pstr_t> ($1); }
	| pstr     { $$ = $1; }
	| evar     { $$ = New refcounted<pstr_t> ($1); }
	| arr 	   { $$ = $1; }
	;

arr: i_arr | g_arr;

g_arr: '('
	{
	  parser->push_parr (New refcounted<parr_mixed_t> ());
	}
	g_arr_list ')'
	{
	  $$ = parser->pop_parr ();
	}
	;

i_arr: i_arr_open 
	{
	  parser->push_parr ($1);
	}
	i_arr_list ')'
	{
	  $$ = parser->pop_parr ();
	}
	;

i_arr_open: T_INT_ARR 	{ $$ = New refcounted<parr_int_t> (); }
	| T_CHAR_ARR 	{ $$ = New refcounted<parr_char_t> (); }
	| T_INT64_ARR 	{ $$ = New refcounted<parr_int64_t> (); }
	| T_INT16_ARR	{ $$ = New refcounted<parr_int16_t> (); }
	| T_UINT_ARR	{ $$ = New refcounted<parr_uint_t> (); }
	| T_UINT16_ARR 	{ $$ = New refcounted<parr_uint16_t> (); }
	;

i_arr_list: number		{ if (!PARR->add ($1)) PARSEFAIL; }
	| i_arr_list ',' number	{ if (!PARR->add ($3)) PARSEFAIL; }
	;

g_arr_list: bvalue		{ PARR->add ($1); }
	| g_arr_list ',' bvalue	{ PARR->add ($3); }
	;

pstr: '"' 
	{
	  PPSTR = New refcounted<pstr_t> ();
	}
	pstr_list '"'
	{
   	  $$ = PPSTR;
	  PPSTR = NULL;
	}
	;

pstr_list: /* empty */ {}
	| pstr_list pstr_el
	;

pstr_el:  T_STR { PPSTR->add ($1); }
	| T_CH  { PPSTR->add ($1); }
	| pvar  { PPSTR->add ($1); }
	| gcode { PPSTR->add ($1); }
	;

pvar: T_BVAR T_VAR '}'
	{
	  $$ = New refcounted<pvar_t> ($2, PLINENO);
	}
	;

gcode: T_BGCODE T_GCODE '}'
	{
	  $$ = New refcounted<gcode_t> ($2, PLINENO);
	}
	|
	T_BGCCE T_GCODE '}'
	{
	  $$ = New refcounted<gcode_t> ($2, PLINENO, true);
	}
	;

str1: '"'
	{
	  PSTR1 = New concatable_str_t ();
	} 
	str1_list '"'
	{
	  $$ = PSTR1->to_str ();
	  delete PSTR1; /* XXX -  probably can't do this */
	  PSTR1 = NULL;
	}
	;

str1_list: /* empty */ {}
	| str1_list str1_el
	;

str1_el: T_STR     { PSTR1->concat ($1); }
	| T_CH     { PSTR1->concat ($1); }
	;

number: T_NUM 
	{ 
	  u_int64_t tmp = strtoull ($1, NULL, 0); 
	  $$ = ($1[0] == '-' ? 0 - tmp : tmp);
	} 
	;

var: T_VAR
	;
