/* -*-fundamental-*- */
/* $Id$ */

%{
#include "pub.h"
#include "pub_parse.h"
#include "parr.h"
#include "pub3.h"
#include "pscalar.h"
#include "pub3parse.h"

%}

%token T_2L_BRACE
%token T_2R_BRACE

%type <str> var str1 bname 
%type <num> number
%type <pvar> pvar evar 
%type <pval> bvalue arr i_arr g_arr
%type <sec> htag htag_list javascript pre b_js_tag
%type <el> ptag 
%type <func> ptag_func
%type <pstr> pstr pstr_sq
%type <arg> arg aarr regex range
%type <parr> i_arr_open
%type <buf> regex_body
%type <nenv> nested_env p3_empty_clause p3_nested_env

/* ------------------------------------------------ */

%token T_P3_EQEQ
%token T_P3_NEQ
%token T_P3_GTEQ
%token T_P3_LTEQ
%token T_P3_OR
%token T_P3_AND
%token T_P3_IF
%token T_P3_FALSE
%token T_P3_BEGIN_EXPR
%token T_P3_INCLUDE
%token T_P3_LOCALS
%token T_P3_UNIVERSALS
%token T_P3_PIPE
%token T_P3_OPEN
%token T_P3_CLOSE
%token T_P3_PRINT
%token T_P3_FOR
%token T_P3_TRUE
%token T_P3_ELIF
%token T_P3_ELSE
%token T_P3_EMPTY
%token T_P3_EVAL
%token T_P3_NULL
%token T_P3_JSON
%token T_P3_CASE
%token T_P3_SWITCH
%token T_P3_DEFAULT

%token <str> T_P3_IDENTIFIER
%token <str> T_P3_INT
%token <str> T_P3_UINT
%token <ch>  T_P3_CHAR
%token <str> T_P3_FLOAT
%token <str> T_P3_STRING
%token <regex> T_P3_REGEX

%type <p3cclist> p3_cond_elifs p3_cond_elifs_opt;
%type <p3cc> p3_cond_else p3_cond_else_opt p3_cond_clause p3_cond_elif;

%type <p3expr> p3_expr p3_logical_AND_expr p3_equality_expr p3_nonparen_expr;
%type <p3expr> p3_relational_expr p3_unary_expr p3_postfix_expr;
%type <p3expr> p3_primary_expr p3_constant p3_additive_expr;
%type <p3expr> p3_multiplicative_expr;
%type <p3expr> p3_integer_constant p3_string p3_string_element;
%type <p3expr> p3_inline_expr p3_regex;
%type <p3expr> p3_inclusive_OR_expr;
%type <p3expr> p3_assignment_expr;
%type <p3expr> p3_conditional_expr;
%type <p3expr> p3_logical_OR_expr;
%type <p3expr> p3_null;
%type <p3str>  p3_string_elements_opt p3_string_elements;
%type <p3dict> p3_bindings_opt p3_bindings p3_dictionary p3_set_arg;
%type <p3bind> p3_binding;
%type <p3include> p3_include_or_load;
%type <el> p3_control p3_for p3_cond p3_include p3_set p3_setl p3_setle;
%type <el> p3_print_or_eval;
%type <els> p3_env p3_zone p3_zone_body p3_zone_body_opt;
%type <elpair> p3_zone_pair;
%type <p3expr> p3_dictref p3_vecref p3_fncall p3_varref p3_recursion;
%type <print> p3_print_or_eval_fn;
%type <p3es> p3_expr_statement p3_statement_opt p3_statement;

%type <relop> p3_relational_op;
%type <p3exprlist> p3_argument_expr_list_opt p3_argument_expr_list;
%type <p3exprlist> p3_tuple p3_list;
%type <p3exprlist> p3_flexi_tuple p3_implicit_tuple;
%type <str> p3_identifier p3_bind_key;
%type <num> p3_character_constant p3_boolean_constant;
%type <dbl> p3_floating_constant;

%type <bl> p3_equality_op p3_additive_op;

%type <p3expr> json_obj json_null;
%type <p3dict> json_dict json_dict_pairs_opt json_dict_pairs;
%type <p3exprlist> json_list json_list_elems_opt json_list_elems;
%type <p3expr> json_scalar json_string json_int json_float json_bool;
%type <p3pair> json_dict_pair;

/* ------------------------------------------------ */

%%
file: hfile 
      	{
	    pub3::pub_parser_t::set_output ($1);
	}
	| json_obj
	{
	    pub3::json_parser_t::set_output ($1);
	}
	;

hfile: p3_zones
	;

p3_zones: /* empty */
	| p3_zones p3_zone
	;

p3_zone:  p3_html_zone 
	| p3_pub_zone
	;

p3_html_zone: p3_html_pre
	| p3_html_script
	| p3_html_block
	;

p3_html_pre: T_P3_PRE_BEGIN p3_html_block T_P3_PRE_END
	{
	   ptr<pub3::html_zone_t> x = 
	     pub3::html_zone_t::alloc (false);
	     x->add ($1);
	     x->add ($2);
	     x->add ($3);
 	     $$ = $1;
	}
	;

p3_html_text: p3_html_atom
	| p3_html_text p3_html_atom
	;

p3_html_atom: T_HTML | T_CH;

p3_html_block: p3_html_element
	| p3_html_block p3_html_element
	;

p3_html_element: p3_html_text
	| p3_inline_expr
	| p3_pub_zone
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
	| p3_html_part
	;

p3_html_part:
	  p3_env 
	{ 
	    PSECTION->hadd_el_list ($1); 
        }
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
	    yy_parse_fail();
	  $$ = POP_PFUNC();
	}
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
	    yy_parse_fail();	    
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
	    yy_parse_fail();
          }
	  $$ = rx;
	};

range: T_RANGE_BEGIN regex_body T_REGEX_END
       {
	 str s;
         ptr<pub_range_t> r = pub_range_t::alloc (*$2, $3, &s);
	 if (!r) {
  	   PWARN(s);
	   yy_parse_fail();
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

binding_char: '=' | ':' ;

binding: bname binding_char bvalue	
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
	| p3_inline_expr { $$ = $1; }
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

i_arr_list: number		{ if (!PARR->add ($1)) yy_parse_fail(); }
	| i_arr_list ',' number	{ if (!PARR->add ($3)) yy_parse_fail(); }
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
	| p3_inline_expr { PPSTR->add (New pub3::pstr_el_t ($1, PLINENO)); }
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

p3_env: T_P3_OPEN p3_zone T_P3_CLOSE
	{
	   $$ = $2;
        }
	;

p3_zone: p3_statement_opt p3_zone_body_opt
       {
         $$ = $2;
	 if ($1) { (*$$)[0] = $1; }
         else { $$->pop_front (); }
       }
       ;


p3_zone_body_opt: /* empty */ 
        { 
 	    $$ = New refcounted<vec<pfile_el_t *> > (); 
	    $$->push_back (NULL);
        }
	| p3_zone_body        { $$ = $1; }
        ;

p3_zone_body: p3_zone_pair
       {
         $$ = New refcounted<vec<pfile_el_t *> > ();
	 $$->push_back (NULL);
	 $1.push ($$);
       } 
       | p3_zone_body p3_zone_pair
       {
         $$ = $1;
	 $2.push ($$);
       }
       ;

p3_zone_pair: p3_control p3_statement_opt
       {
         $$.first = $1;
	 $$.second = $2;
       }
       ;


/* XXX - might want to fix the nested_env assignment, implemented 
 * as such to avoid another object and level of redirection.  The
 * downside is that scoping rules aren't followed, since the nested
 * env is flattened into the parent env.
 */
p3_control: p3_for { $$ = $1 ;}
	      | p3_cond { $$ = $1; }
	      | p3_set { $$ = $1; }
	      | p3_setl { $$ = $1; }
	      | p3_setle { $$ = $1; }
	      | p3_include { $$ = $1; }
	      | p3_print_or_eval { $$ = $1; }
	      | nested_env { $$ = New pfile_nested_env_t ($1); }
              | ';' { $$ = NULL; }
	      ;

p3_statement_opt: /*empty*/ { $$ = NULL; }
	      | p3_statement
	      ;

p3_statement: p3_expr_statement { $$ = $1; }
	      ;

p3_expr_statement: p3_expr
	      {
	         $$ = New pub3::expr_statement_t ($1, PLINENO);
	      }
	      ;

p3_expr: p3_assignment_expr;

p3_assignment_expr: p3_conditional_expr { $$ = $1; }
	       | p3_unary_expr '=' p3_assignment_expr
	       {
                  $$ = New refcounted<pub3::expr_assignment_t> ($1, $3, NULL);
	       }
	       ;

p3_conditional_expr: p3_logical_OR_expr;

p3_logical_OR_expr: p3_logical_AND_expr
	       {
	          $$ = $1;
	       }
	       | p3_logical_OR_expr T_P3_OR p3_logical_AND_expr
	       {
	          $$ = New refcounted<pub3::expr_OR_t> ($1, $3, PLINENO);
               }
	       ;

p3_logical_AND_expr: p3_inclusive_OR_expr
               { 
	          $$ = $1 ; 
	       }
	       | p3_logical_AND_expr T_P3_AND p3_inclusive_OR_expr
               { 
	          $$ = New refcounted<pub3::expr_AND_t> ($1, $3, PLINENO);
	       }
	       ;

p3_inclusive_OR_expr: p3_equality_expr
	       {
	          $$ = $1;
	       }
	       | p3_inclusive_OR_expr T_P3_PIPE p3_equality_expr
	       {
	          if (!$3->unshift_argument ($1)) {
		     PWARN("Cannot push argument onto non-function");
		     yy_parse_fail();
		  }
		  $$ = $3;
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
	     p3_multiplicative_expr
	   {
	     $$ = $1;
	   }
	   | p3_additive_expr p3_additive_op p3_multiplicative_expr
	   {
	     $$ = New refcounted<pub3::expr_add_t> ($1, $3, $2, PLINENO);
	   }
	   ;

p3_multiplicative_expr:
             p3_unary_expr
           {
	      $$ = $1;
	   }
	   | p3_multiplicative_expr '%' p3_unary_expr
	   {
	     $$ = New refcounted<pub3::expr_mod_t> ($1, $3, PLINENO);
	   }
	   | p3_multiplicative_expr '*' p3_unary_expr
	   {
	     $$ = New refcounted<pub3::expr_mult_t> ($1, $3, PLINENO);
	   }
	   | p3_multiplicative_expr '/' p3_unary_expr
	   {
             $$ = New refcounted<pub3::expr_div_t> ($1, $3, PLINENO);
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
	      $$ = New refcounted<pub3::expr_NOT_t> ($2, PLINENO);
           }
	   ;

p3_dictref:  p3_postfix_expr '.' p3_identifier
	   {
	      $$ = New refcounted<pub3::expr_dictref_t> ($1, $3, PLINENO);
	   }
	   ;

p3_vecref: p3_postfix_expr '['  p3_expr ']'
	   {
	      $$ = New refcounted<pub3::expr_vecref_t> ($1, $3, PLINENO);
	   }
	   ;

p3_fncall: p3_identifier '(' p3_argument_expr_list_opt ')' 
	   {
	      /* Allocate a stub at first, which will be resolved 
	       * into the true function either at evaluation time
	       * (in the pub command line client) or upon conversion
	       * from XDR (in OKWS services)
	       */
	      $$ = New refcounted<pub3::runtime_fn_stub_t> ($1, $3, PLINENO);
           }
	   ;

p3_varref: p3_identifier 
           { 
              /* See comment in pub3expr.h -- this identifier might be
	       * a function call in a pipeline; we just don't know yet!
	       */
	      $$ = New refcounted<pub3::expr_varref_or_rfn_t> ($1, PLINENO);
	   }
	   ;

p3_recursion: '(' p3_expr ')' { $$ = $2; };

p3_postfix_expr:
             p3_primary_expr { $$ = $1; } 
	   | p3_dictref      { $$ = $1; }
	   | p3_vecref       { $$ = $1; }
	   | p3_fncall       { $$ = $1; }
	   | p3_dictionary   { $$ = $1; }
	   | p3_list         { $$ = $1; }
	   | p3_null         { $$ = $1; }
	   ;

p3_null : T_P3_NULL { $$ = pub3::expr_null_t::alloc (PLINENO); }
	;

p3_primary_expr: p3_varref   { $$ = $1; }
           | p3_constant     { $$ = $1; }
	   | p3_string       { $$ = $1; }
	   | p3_recursion    { $$ = $1; }
	   | p3_regex        { $$ = $1; }
	   ;

p3_nonparen_expr: 
             p3_dictionary  { $$ = $1; }
	   | p3_list        { $$ = $1; }
	   | p3_constant    { $$ = $1; }
	   | p3_string      { $$ = $1; }
	   | p3_regex       { $$ = $1; }
	   | p3_varref      { $$ = $1; }
	   ;

p3_regex: T_P3_REGEX
	  {
	     str err;
	     ptr<rxx> x = 
                 pub3::rxx_factory_t::compile ($1.regex, $1.opts, &err);
	     if (err) {
               PWARN(err);
	       yy_parse_fail();
	     }
	     $$ = New refcounted<pub3::expr_regex_t> 
                (x, $1.regex, $1.opts, PLINENO);
	  }
	  ;

p3_argument_expr_list_opt:           
           { 
              $$ = New refcounted<pub3::expr_list_t> (PLINENO); 
           }
           | p3_argument_expr_list   { $$ = $1; }
           ;

p3_argument_expr_list: p3_expr
           {
	      $$ = New refcounted<pub3::expr_list_t> (PLINENO);
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
	      $$ = pub3::expr_bool_t::alloc ($1);
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
	      yy_parse_fail();
	   }
	   $$ = New refcounted<pub3::expr_int_t> (i);
	}
	| T_P3_UINT
	{
	   u_int64_t u = 0;
	   if (!convertuint ($1, &u)) {
	      strbuf b ("Cannot conver '%s' to unsigned int", $1.cstr ());
	      yy_parse_fail();
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
	      yy_parse_fail();
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

p3_binding: p3_bind_key p3_bindchar p3_expr
	{
	   $$ = New nvpair_t ($1, $3);
	}
	;

p3_bind_key: p3_identifier { $$ = $1; }
        | p3_string        { $$ = $1->to_str (); }
	;

p3_bindchar: ':' | '=' ;
	

p3_string: '"' p3_string_elements_opt '"'
        {
           $$ = $2->compact ();
	}
	|
	'\'' p3_string_elements_opt '\''
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

p3_list: '[' p3_argument_expr_list_opt ']' { $$ = $2; } ;

p3_tuple: '(' p3_argument_expr_list_opt ')' { $$ = $2; } ;

/*
 * A flexituple is a tuple that has the flexibility to drop the surrounding
 * parentheses.  The p3_tuple is the straight-ahead tuple, while the
 * implicit_tuple allows you to get away without the wrapper parens
 * but cripples the flexibility of objects within the tuple (in particular
 * to expressions wrapped in '()' since that would be ambiguous..
 */
p3_flexi_tuple: p3_tuple
	| p3_implicit_tuple
	;

p3_implicit_tuple: 
          p3_nonparen_expr
	{
	  ptr<pub3::expr_list_t> l = 
             New refcounted<pub3::expr_list_t> (PLINENO);
	  l->push_back ($1);
          $$ = l;
	}
	| p3_implicit_tuple ',' p3_nonparen_expr
	{
	  $1->push_back ($3);
      	  $$ = $1;
	}
	;

p3_include_or_load: 
          T_P3_INCLUDE { $$ = New pub3::include_t (PLINENO); }
        | T_P3_LOAD    { $$ = New pub3::load_t (PLINENO); }
	;
		   
	
p3_include: p3_include_or_load p3_flexi_tuple 
        {
           str err;
           pub3::include_t *f = $1;
	   if (!f->add ($2)) {
	     yy_parse_fail();
	   }
           $$ = f;
	};

p3_set_arg: '(' p3_dictionary ')' { $$ = $2; }
	    | p3_dictionary { $$ = $1; }
	    ;

p3_set: T_P3_SET p3_set_arg
	{
           pub3::set_func_t *f = New pub3::set_func_t (PLINENO);
           f->add ($2);   
	   $$ = f;
	}
	;

p3_setl: T_P3_SETL p3_set_arg
	{
           pfile_set_func_t *f = New pfile_set_local_func_t (PLINENO);
           f->add ($2);   
	   $$ = f;
        }
        ;

p3_setle: T_P3_SETLE p3_set_arg
	{
           pfile_set_func_t *f = New pub3::setle_func_t (PLINENO);
           f->add ($2);   
	   $$ = f;
        }
	;

p3_nested_env: nested_env { $$ = $1; }
	| 
        '{' p3_zone '}'
	{
 	  pfile_html_sec_t *s = New pfile_html_sec_t (PLINENO);
	  s->hadd_el_list ($2);
	  $$ = New refcounted<nested_env_t> (s);
	}
	;

p3_for: T_P3_FOR p3_flexi_tuple p3_nested_env p3_empty_clause 
        {
	    pub3::for_t *f = New pub3::for_t (PLINENO);
	    if (!f->add ($2)) {
	      yy_parse_fail();
	    }
	    f->add_env ($3);
	    f->add_empty ($4);
	    $$ = f;
	};

p3_print_or_eval_fn: 
         T_P3_PRINT { $$ = New pub3::print_t (false, PLINENO); }
       | T_P3_EVAL { $$ = New pub3::print_t (true, PLINENO); }
       ;

p3_print_or_eval: p3_print_or_eval_fn p3_flexi_tuple
       {
           pub3::print_t *p = $1;
	   if (!p->add ($2)) {
	     PWARN("bad arguments passed to print");
	     yy_parse_fail();
	   }
	   $$ = p;
       }
       ;

p3_cond: T_P3_COND p3_cond_clause p3_cond_elifs_opt p3_cond_else_opt
       {
          pub3::cond_t *c = New pub3::cond_t (PLINENO);
	  c->add_clause ($2);
	  c->add_clauses ($3);
	  c->add_clause ($4);
	  $$ = c;
       }
       ;

p3_cond_elifs_opt: /*empty*/ { $$ = NULL; }
       | p3_cond_elifs { $$ = $1; }
       ;

p3_cond_else_opt: /* empty */ { $$ = NULL; }
       | p3_cond_else { $$ = $1; }
       ;

p3_cond_elifs: p3_cond_elif 
       {
           $$ = New refcounted<pub3::cond_clause_list_t> ();
	   $$->push_back ($1);
       }
       | p3_cond_elifs p3_cond_elif
       {
           $$ = $1;
	   $$->push_back ($2);
       }
       ;

p3_cond_elif: T_P3_ELIF	p3_cond_clause { $$ = $2; } ;

p3_cond_else: T_P3_ELSE p3_nested_env
       {
	   ptr<pub3::cond_clause_t> c = pub3::cond_clause_t::alloc (PLINENO);
	   c->add_env ($2);
	   $$ = c;
       }
       ;

p3_cond_clause: '(' p3_expr ')' p3_nested_env
       {
	    ptr<pub3::cond_clause_t> c = pub3::cond_clause_t::alloc (PLINENO);
	    c->add_expr ($2);
	    c->add_env ($4);
	    $$ = c;
       }
       ;

p3_empty_clause: 
         /* empty */                   { $$ = NULL; }
	 | T_P3_EMPTY p3_nested_env    { $$ = $2; }
 	 ;

		  
/*----------------------------------------------------------------------- */

json_obj:   json_dict { $$ = $1; }
      | json_list { $$ = $1; }
      | json_scalar  { $$ = $1; }
      | json_null { $$ = $1; }
      ;

json_dict: '{' json_dict_pairs_opt '}'  { $$ = $2; } ;

json_dict_pairs_opt: 
        /* empty */ 
      { $$ = New refcounted<pub3::expr_dict_t> (yy_get_json_lineno ()); }
      | json_dict_pairs { $$ = $1; }
      ;

json_dict_pairs: 
        json_dict_pair 
      {
         ptr<pub3::expr_dict_t> d = 
	    New refcounted<pub3::expr_dict_t> (yy_get_json_lineno ()); 
         d->add ($1);
	 $$ = d;
      }
      | json_dict_pairs ',' json_dict_pair
      {
         $1->add ($3);
	 $$ = $1;
      }
      ;

json_dict_pair: T_P3_STRING ':' json_obj
      {
         $$ = New refcounted<pub3::pair_t> ($1, $3);
      }
      ;

json_null : T_P3_NULL { $$ = pub3::expr_null_t::alloc (yy_get_json_lineno ()); }
	  ;

json_list: '[' json_list_elems_opt ']' { $$ = $2; } ;

json_list_elems_opt: /* empty */ 
      {
         $$ = New refcounted<pub3::expr_list_t> (yy_get_json_lineno ());
      }
      | json_list_elems { $$ = $1; }
      ;

json_list_elems: json_obj
      {
         $$ = New refcounted<pub3::expr_list_t> (yy_get_json_lineno ());
         $$->push_back ($1);
      }
      | json_list_elems ',' json_obj
      {
         $1->push_back ($3);
	 $$ = $1;
      }
      ;

json_scalar: json_bool
      | json_int
      | json_float
      | json_string
      ;

json_string: T_P3_STRING { $$ = New refcounted<pub3::expr_str_t> ($1); }
json_bool : p3_boolean_constant { $$ = pub3::expr_bool_t::alloc ($1); } ;

json_float: p3_floating_constant 
    { $$ = New refcounted<pub3::expr_double_t> ($1); } ;

json_int  : p3_integer_constant { $$ = $1; } ;


/*----------------------------------------------------------------------- */

//
// New proposal for p3 sections:
//
//
//  p3_zone = p3_assignment_opt (p3_control p3_assignment_opt)*
//
//  p3_zone = p3_assignment_opt |
//  	    p3_zone p3_control p3_assignment_opt ;
//
//
//  p3_assignment_opt = p3_assignment | /* empty */ ;
//
//  p3_assignment = p3_statement |
//  		   p3_assignment '=' p3_statement
//
//
//  p3_control = for | cond | set | setl | include | nested | ';'
//
//

