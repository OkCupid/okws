/* -*-fundamental-*- */
/* $Id$ */

%{
#include "pub.h"
#include "pub_parse.h"
#include "parr.h"
#include "pub3.h"
#include "pscalar.h"

%}

%token <str> T_NUM
%token <str> T_HNAM
%token <str> T_HVAL
%token <str> T_STR
%token <str> T_VAR
%token <ch>  T_CH
%token <str> T_ETAG
%token <str> T_BJST
%token <str> T_EJS
%token <str> T_HTML
%token <str> T_BTAG
%token <str> T_BPRE
%token <str> T_EPRE
%token <str> T_REGEX_BODY
%token <str> T_REGEX_END

%token T_PTINCLUDE
%token T_PTLOAD
%token T_PTINCLIST
%token T_PTSET
%token T_PTSETL
%token T_PTSWITCH
%token T_EPTAG
%token T_BVAR
%token T_BCONF
%token T_EJS_SILENT
%token T_BJS_SILENT
%token T_2L_BRACE
%token T_2R_BRACE

%token T_REGEX_BEGIN
%token T_RANGE_BEGIN

%token T_INT_ARR
%token T_UINT_ARR
%token T_CHAR_ARR
%token T_INT16_ARR
%token T_UINT16_ARR
%token T_INT64_ARR

%type <str> var str1 bname 
%type <num> number
%type <pvar> pvar evar 
%type <pval> bvalue arr i_arr g_arr
%type <sec> htag htag_list javascript pre b_js_tag
%type <el> ptag p3_forloop p3_cond p3_include
%type <func> ptag_func
%type <pstr> pstr pstr_sq
%type <arg> arg aarr regex range
%type <parr> i_arr_open
%type <buf> regex_body
%type <nenv> nested_env empty_clause

/* ------------------------------------------------ */

%token T_P3_EQEQ
%token T_P3_NEQ
%token T_P3_GTEQ
%token T_P3_LTEQ
%token T_P3_OR
%token T_P3_AND
%token T_P3_COND
%token T_P3_FALSE
%token T_P3_BEGIN_EXPR
%token T_P3_END_EXPR
%token T_P3_INCLUDE
%token T_P3_CLOSETAG

%token T_P3_FOR
%token T_P3_TRUE

%token <str> T_P3_IDENTIFIER
%token <str> T_P3_INT
%token <str> T_P3_UINT
%token <ch>  T_P3_CHAR
%token <str> T_P3_FLOAT
%token <str> T_P3_STRING

%type <p3cclist> p3_cond_clause_list;
%type <p3cc> p3_cond_clause;

%type <p3expr> p3_expr p3_logical_AND_expr p3_equality_expr;
%type <p3expr> p3_relational_expr p3_unary_expr p3_postfix_expr;
%type <p3expr> p3_primary_expr p3_constant p3_additive_expr;
%type <p3expr> p3_integer_constant p3_string p3_string_element;
%type <p3expr> p3_inline_expr;
%type <p3str>  p3_string_elements_opt p3_string_elements;
%type <p3dict> p3_bindings_opt p3_bindings;
%type <p3bind> p3_binding;
%type <p3expr> p3_dictionary;

%type <relop> p3_relational_op;
%type <p3exprlist> p3_argument_expr_list_opt p3_argument_expr_list p3_vector;
%type <str> p3_identifier;
%type <num> p3_character_constant p3_boolean_constant;
%type <dbl> p3_floating_constant;


%type <bl> p3_equality_op p3_additive_op;

/* ------------------------------------------------ */

%%
file: hfile {}
	| conffile {}
	;

conffile: T_BCONF aarr {}
	;

hfile: html 
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
	| p3_inline_expr
        { 
	    PSECTION->hadd (New pub3::inline_var_t ($1, PLINENO)); 
        }
	;

nested_env: T_2L_BRACE
	{ 
 	  pfile_html_sec_t *s = New pfile_html_sec_t (PLINENO);
	  PFILE->push_section (s); 
	} 
        html T_2R_BRACE 
 	{
	  pfile_sec_t *s = PFILE->pop_section ();
	  $$ = New refcounted<nested_env_t> (s);
	}
	;

evar:     pvar
	;
	
ptag: ptag_func 
	{
 	  PUSH_PFUNC ($1);
	}
	ptag_list ptag_close
	{
	  if (PFUNC->validate ()) {
	    if (parser->do_explore ()) {
	      PFUNC->explore (EXPLORE_PARSE);
	    }
	  } else
	    PARSEFAIL;
	  $$ = POP_PFUNC();
	}
	| p3_forloop
	| p3_cond
	| p3_include
	;

ptag_close: ';' T_EPTAG
	| T_EPTAG
	;

ptag_func: T_PTINCLUDE  
	{ 
	   switch (parser->get_include_version ()) {
	   case XPUB_V2:
	      $$ = New pfile_include2_t (PLINENO); 
	      break;
	   case XPUB_V1:
	      $$ = New pfile_include_t (PLINENO);
 	      break;
	   default:
	      panic ("unexpected PUB version (not 1 or 2)\n");
	      break;
	   }
        }
	| T_PTSET	{ $$ = New pfile_set_func_t (PLINENO); }
	| T_PTSETL      { $$ = New pfile_set_local_func_t (PLINENO); }
	| T_PTSWITCH	{ $$ = New pfile_switch_t (PLINENO); }
	| T_PTINCLIST	{ $$ = New pfile_inclist_t (PLINENO); }
	| T_PTLOAD      { $$ = New pfile_load_t (PLINENO); }
	;

empty_clause: 
         /* empty */       { $$ = NULL; }
	 | ','  nested_env { $$ = $2; }
 	 ;



e_js_tag: T_EJS		{ PSECTION->add ($1); }
	| T_EJS_SILENT	{}
	;

javascript: b_js_tag js_code e_js_tag
	{
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
	| T_BJS_SILENT 
	{
	  /* we still need this here, even though it will most
	   * likely be empty
 	   */
	  PFILE->push_section (New pfile_html_sec_t (PLINENO));
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
	  if ($5[0] != '>') {
	    PSECTION->htag_space ();
 	  }
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
	| pstr_sq
	{
 	   PSECTION->add ('\'');
	   PSECTION->add (New pfile_pstr_t ($1)); 
 	   PSECTION->add ('\'');
	}
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

regex_body: 
	    {
		ptr<strbuf> b = New refcounted<strbuf> ();
		*b << "";
		$$ = b;
	    }
	    | regex_body T_REGEX_BODY
	    {
	       $1->cat ($2.cstr (), true);
	       $$ = $1;
	    };

regex:	T_REGEX_BEGIN regex_body T_REGEX_END 
	{
	  ptr<pub_regex_t> rx = New refcounted<pub_regex_t> (*$2, $3);
	  str s;
	  if (!rx->compile (&s)) {
            PWARN(s);
	    PARSEFAIL;
          }
	  $$ = rx;
	};

range: T_RANGE_BEGIN regex_body T_REGEX_END
       {
	 str s;
         ptr<pub_range_t> r = pub_range_t::alloc (*$2, $3, &s);
	 if (!r) {
  	   PWARN(s);
	   PARSEFAIL;
	 }
         $$ = r;
       };

arg: /* empty */  { $$ = New refcounted<pval_null_t> (); }
	| var     { $$ = New refcounted<pstr_t> ($1); }
	| pstr    { $$ = $1; }
	| number  { $$ = New refcounted<pint_t> ($1); }
	| aarr    { $$ = $1; }
	| pvar    { $$ = New refcounted<pstr_t> ($1); }
	| nested_env { $$ = $1; }
	| regex   { $$ = $1; }
	| range   { $$ = $1; }
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

pstr_sq: '\'' 
	{
	  PPSTR = New refcounted<pstr_t> ();
	}
	pstr_list '\''
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
	;

pvar: T_BVAR T_VAR '}'
	{
	  $$ = New refcounted<pvar_t> ($2, PLINENO);
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


/*----------------------------------------------------------------------- */
/* Below follows the new support for generic sexpr's, which will appear
 * in {% cond %} statements and also filters.
 */

p3_expr: p3_logical_AND_expr
	       {
	          $$ = $1;
	       }
	       | p3_expr T_P3_OR p3_logical_AND_expr
	       {
	          $$ = New refcounted<pub3::expr_OR_t> ($1, $3);
               }
	       ;

p3_logical_AND_expr: p3_equality_expr
               { 
	          $$ = $1 ; 
	       }
	       | p3_logical_AND_expr T_P3_AND p3_equality_expr
               { 
	          $$ = New refcounted<pub3::expr_AND_t> ($1, $3);
	       }
	       ;

p3_equality_expr: p3_relational_expr
		{
  		   $$ = $1;
		}
	 	| p3_equality_expr p3_equality_op p3_relational_expr
		{
		   $$ = New refcounted<pub3::expr_EQ_t> ($1, $3, $2, PLINENO);
		}
		;	

p3_equality_op: T_P3_EQEQ   { $$ = true; }
		| T_P3_NEQ  { $$ = false; }
		;

p3_relational_expr: 
             p3_additive_expr
           {
	      $$ = $1;
	   }
           | p3_relational_expr p3_relational_op p3_additive_expr
	   {
	      $$ = New refcounted<pub3::expr_relation_t> ($1, $3, $2, PLINENO);
           }
	   ;	
	   
p3_relational_op: 
             '<'        { $$ = XPUB3_REL_LT; }
	   | '>'        { $$ = XPUB3_REL_GT; }
	   | T_P3_GTEQ  { $$ = XPUB3_REL_GTE; }
           | T_P3_LTEQ  { $$ = XPUB3_REL_LTE; }
	   ;

p3_additive_expr:
             p3_unary_expr
           {
	      $$ = $1;
	   }
	   | p3_additive_expr p3_additive_op p3_unary_expr
	   {
	     $$ = New refcounted<pub3::expr_add_t> ($1, $3, $2, PLINENO);
	   }
	   ;

p3_additive_op:
	     '-'       { $$ = false; }
	   | '+'       { $$ = true; }
	   ;

p3_unary_expr: 
             p3_postfix_expr
	   {
	      $$ = $1;
           }
           | '!' p3_unary_expr
	   {
	      $$ = New refcounted<pub3::expr_NOT_t> ($2);
           }
	   ;

p3_postfix_expr:
           p3_primary_expr 
	   {
	      $$ = $1;
           } 
	   | p3_postfix_expr '.' p3_identifier
	   {
	      $$ = New refcounted<pub3::expr_dictref_t> ($1, $3, PLINENO);
           } 
	   | p3_postfix_expr '['  p3_expr ']'
	   {
	      $$ = New refcounted<pub3::expr_vecref_t> ($1, $3, PLINENO);
           } 
	   | p3_identifier '(' p3_argument_expr_list_opt ')' 
	   {
	      $$ = New refcounted<pub3::runtime_fn_t> ($1, $3, PLINENO);
           }
	   | p3_dictionary
           {
	      $$ = $1;
           }
	   ;

p3_primary_expr: p3_identifier 
           { 
	      $$ = New refcounted<pub3::expr_varref_t> ($1, PLINENO);
	   }
           | p3_constant       
	   { 
	      $$ = $1;
           }
	   | p3_string      
	   { 
              $$ = $1;
	   }
	   | '(' p3_expr ')'   { $$ = $2;   }
	   ;

p3_argument_expr_list_opt:           { $$ = NULL; }
           | p3_argument_expr_list   { $$ = $1; }
           ;

p3_argument_expr_list: p3_expr
           {
	      $$ = New refcounted<pub3::expr_list_t> ();
	      $$->push_back ($1);
	   }
           | p3_argument_expr_list ',' p3_expr
	   {
	      $$ = $1;
	      $$->push_back ($3);
	   }
	   ;

p3_constant: 
           p3_integer_constant
           {
	      $$ = $1;
	   }
           | p3_character_constant
           {
	      $$ = New refcounted<pub3::expr_int_t> ($1); 
	   }
	   | p3_floating_constant
	   {
	      $$ = New refcounted<pub3::expr_double_t> ($1); 
	   }
           | p3_boolean_constant
	   {
	      $$ = New refcounted<pub3::expr_int_t> ($1);
	   }
	   ;

p3_identifier: T_P3_IDENTIFIER { $$ = $1; } ;

p3_boolean_constant: 
          T_P3_TRUE { $$ = 1; }
        | T_P3_FALSE { $$ = 0; }

p3_integer_constant: T_P3_INT
        {
	   int64_t i = 0;
	   if (!convertint ($1, &i)) {
	      strbuf b ("Cannot convert '%s' to int", $1.cstr ());
	      PARSEFAIL;
	   }
	   $$ = New refcounted<pub3::expr_int_t> (i);
	}
	| T_P3_UINT
	{
	   u_int64_t u = 0;
	   if (!convertuint ($1, &u)) {
	      strbuf b ("Cannot conver '%s' to unsigned int", $1.cstr ());
	      PARSEFAIL;
           }
	   if (u <= u_int64_t (INT64_MAX)) {
	     $$ = New refcounted<pub3::expr_int_t> (u);
           } else {
	     $$ = New refcounted<pub3::expr_uint_t> (u);
           }
	}
	;

p3_floating_constant: T_P3_FLOAT
        {
	   double d = 0;
	   if (!convertdouble ($1, &d)) {
	      strbuf b ("Cannot convert '%s' to double", $1.cstr ());
	      PARSEFAIL;
           }
	   $$ = d;
	}
	;

p3_character_constant: T_P3_CHAR { $$ = $1; };

p3_dictionary: '{' p3_bindings_opt '}' { $$ = $2; }
	       ;

p3_bindings_opt: 
         /* empty */     
        { $$ = New refcounted<pub3::expr_dict_t> (PLINENO); }
	| p3_bindings 
        { $$ = $1; }
	;

p3_bindings: p3_binding
        {
	   ptr<pub3::expr_dict_t> d = 
               New refcounted<pub3::expr_dict_t> (PLINENO);
           d->add ($1);
	   $$ = d;	 
	}
	| p3_bindings ',' p3_binding
	{
	   $1->add ($3);
           $$ = $1;
	}
	;

p3_binding: p3_identifier p3_bindchar p3_expr
	{
	   $$ = New nvpair_t ($1, $3);
	}
	;

p3_bindchar: ':' | '=' ;
	

p3_string: '"' p3_string_elements_opt '"'
        {
           $$ = $2->compact ();
	}
	;

p3_string_elements_opt:
          /* empty */
        {
           $$ = New refcounted<pub3::expr_shell_str_t> ("", PLINENO);
	}
	| p3_string_elements
	{
	   $$ = $1;
	}
	;

p3_string_elements: 
          p3_string_element 
        { 
           $$ = New refcounted<pub3::expr_shell_str_t> ($1, PLINENO);
	}
        | p3_string_elements p3_string_element
	{
	   $$->add ($2);
	}
	;

p3_string_element: 
          T_P3_STRING { $$ = New refcounted<pub3::expr_str_t> ($1); }
	| T_P3_CHAR 
	{ 
	   $$ = New refcounted<pub3::expr_str_t> (strbuf ("%c", $1));
	}
	| p3_inline_expr 
        { 
           $$ = $1; 
        }
	;

p3_inline_expr: 
        T_P3_BEGIN_EXPR p3_expr '}' { $$ = $2; }
        ;

p3_vector: '(' p3_argument_expr_list_opt ')' { $$ = $2; } ;
	
p3_include: T_P3_INCLUDE p3_vector T_P3_CLOSETAG
        {
           str err;
           pub3::include_t *f = New pub3::include_t (PLINENO);
	   if (!f->add ($2)) {
	     PARSEFAIL;
	   }
           $$ = f;
	};

p3_forloop: T_P3_FOR p3_vector nested_env empty_clause T_P3_CLOSETAG
	 {
	    pub3::for_t *f = New pub3::for_t (PLINENO);
	    if (!f->add ($2)) {
	      PARSEFAIL;
	    }
	    f->add_env ($3);
	    f->add_empty ($4);
	    $$ = f;
	 };

p3_cond: T_P3_COND p3_cond_clause_list T_P3_CLOSETAG
      {
         pub3::cond_t *c = New pub3::cond_t (PLINENO);
	 c->add_clauses ($2);
	 $$ = c;
      }
      ;

p3_cond_clause_list: 
          p3_cond_clause
	  {
	     $$ = New refcounted<pub3::cond_clause_list_t> ();
	     $$->push_back ($1);
          }
	  | p3_cond_clause_list ',' p3_cond_clause
	  {
	     $1->push_back ($3);
  	     $$ = $1;
          }
	  ;

p3_cond_clause: '(' p3_expr ')' nested_env
         {
	    ptr<pub3::cond_clause_t> c = pub3::cond_clause_t::alloc (PLINENO);
	    c->add_expr ($2);
	    c->add_env ($4);
	    $$ = c;
	 }
	 ;
		  
/*----------------------------------------------------------------------- */
