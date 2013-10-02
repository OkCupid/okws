/* -*-fundamental-*- */
/* $Id$ */

%{
#include "okformat.h"

#include "pub.h"
#include "pub_parse.h"
#include "pub3.h"
#include "pscalar.h"
#include "pub3parse.h"
#include "okformat.h"

#ifdef __clang__
// bison issues statements with extra parenthesies
#pragma clang diagnostic ignored "-Wparentheses-equality"
#endif  // __clang__

%}

%token T_2L_BRACE
%token T_2R_BRACE

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
%token T_P3_LOAD
%token T_P3_LOCALS
%token T_P3_UNIVERSALS
%token T_P3_GLOBALS
%token T_P3_PIPE
%token T_P3_OPEN
%token T_P3_CLOSE
%token T_P3_PRINT
%token T_P3_FOR
%token T_P3_WHILE
%token T_P3_TRUE
%token T_P3_ELIF
%token T_P3_ELSE
%token T_P3_EMPTY
%token T_P3_NULL
%token T_P3_CASE
%token T_P3_SWITCH
%token T_P3_DEF
%token T_P3_LAMBDA
%token T_P3_BREAK
%token T_P3_RETURN
%token T_P3_CONTINUE
%token T_P3_EXIT
%token T_P3_HTML_HEREDOC
%token T_P3_PUB_HEREDOC
%token T_P3_BCALL_OPEN
%token T_P3_BCALL_CLOSE

%token <str> T_P3_IDENTIFIER
%token <str> T_P3_INT
%token <str> T_P3_UINT
%token <ch>  T_P3_CHAR
%token <str> T_P3_FLOAT
%token <str> T_P3_STRING
%token <regex> T_P3_REGEX
%token <str> T_P3_HTML
%token <ch> T_P3_HTML_CH

%token <str> T_P3_WSS_BOUNDARY

%type <p3iclist> p3_elifs p3_elifs_opt;
%type <p3ic> p3_if_clause p3_elif p3_else p3_else_opt;

%type <p3expr> p3_expr p3_logical_AND_expr p3_equality_expr p3_nonparen_expr;
%type <p3expr> p3_relational_expr p3_unary_expr p3_postfix_expr;
%type <p3expr> p3_primary_expr p3_constant p3_additive_expr;
%type <p3expr> p3_multiplicative_expr;
%type <p3expr> p3_integer_constant p3_string p3_string_element;
%type <p3expr> p3_regex;
%type <p3expr> p3_inclusive_OR_expr;
%type <p3expr> p3_assignment_expr;
%type <p3expr> p3_conditional_expr;
%type <p3expr> p3_logical_OR_expr;
%type <p3expr> p3_bind_value_opt;
%type <p3expr> p3_null p3_heredoc p3_heredoc_pub p3_heredoc_html;
%type <p3expr> p3_return_value;
%type <p3str>  p3_string_elements_opt p3_string_elements;
%type <p3dict> p3_bindings_opt p3_bindings p3_dictionary;
%type <p3bl> p3_bindlist p3_bindlist_bindings p3_locals_arg;
%type <p3bind> p3_binding;
%type <p3include> p3_include_or_load;
%type <p3statement> p3_control p3_for p3_if p3_include p3_locals p3_while;
%type <p3statement> p3_universals p3_print p3_fndef p3_switch p3_globals;
%type <p3statement> p3_break p3_return p3_continue p3_exit;
%type <p3expr> p3_dictref p3_vecref p3_fncall p3_varref p3_recursion;
%type <p3statement> p3_expr_statement p3_statement_opt p3_statement;
%type <p3cl> p3_switch_cases;
%type <p3case> p3_switch_case ;
%type <p3pair> p3_pub_zone_pair;
%type <lineno> p3_t_for p3_t_if p3_t_print p3_t_locals p3_t_universals;
%type <lineno> p3_t_while;
%type <lineno> p3_t_globals;

%type <relop> p3_relational_op;
%type <p3exprlist> p3_argument_expr_list_opt p3_argument_expr_list;
%type <p3exprlist> p3_tuple p3_list;
%type <p3exprlist> p3_flexi_tuple p3_implicit_tuple;
%type <str> p3_identifier p3_bind_key;
%type <p3strv> p3_identifier_list p3_identifier_list_opt;
%type <num> p3_boolean_constant;
%type <dbl> p3_floating_constant;
%type <p3expr> p3_constant_or_string p3_case_key_opt;
%type <p3strbuf> p3_string_constant p3_string_constant_inner;
%type <str> p3_string_constant_element;
 
%type <bl> p3_equality_op p3_additive_op;

%type <p3expr> json_obj json_null;
%type <p3dict> json_dict json_dict_pairs_opt json_dict_pairs;
%type <p3exprlist> json_list json_list_elems_opt json_list_elems;
%type <p3expr> json_scalar json_string json_int json_float json_bool;
%type <p3bind> json_dict_pair;

%type <p3zone> p3_html_zone p3_html_zone_inner;
%type <p3zone> p3_html_blocks p3_html_block;
%type <p3zone> p3_pub_zone p3_pub_zone_inner;
%type <p3expr> p3_inline_expr;
%type <p3zp> p3_pub_zone_body_opt p3_pub_zone_body;
%type <p3zone> p3_nested_zone;
%type <p3zone> p3_empty_clause;
%type <p3lambda> p3_lambda p3_fndef_body;


/* ------------------------------------------------ */

%%
file: p3_html_zone_inner
      	{
	    pub3::parser_t::current ()->set_zone_output ($1);
	}
	| json_obj
	{
	    pub3::parser_t::current ()->set_expr_output ($1);
	}
	;

p3_html_zone: T_2L_BRACE p3_html_zone_inner T_2R_BRACE { $$ = $2; }
        ;

p3_html_zone_inner: p3_html_blocks;

p3_html_blocks:  { $$ = NULL; }
	| p3_html_blocks p3_html_block
	{
	   ptr<pub3::zone_html_t> z = pub3::zone_html_t::alloc ($1);
           z->add ($2);
	   $$ = z;
	}
	| p3_html_blocks T_P3_HTML
	{
	   ptr<pub3::zone_html_t> z = pub3::zone_html_t::alloc ($1);
	   z->add ($2);
	   $$ = z;
	}
	| p3_html_blocks T_P3_HTML_CH
	{
	   ptr<pub3::zone_html_t> z = pub3::zone_html_t::alloc ($1);
	   z->add ($2);
	   $$ = z;
        }
	| p3_html_blocks T_P3_WSS_BOUNDARY
	{
	  ptr<pub3::zone_html_t> z = pub3::zone_html_t::alloc ($1);
	  z->add_boundary ($2);
	  $$ = z;
	}
	;

p3_html_block: p3_inline_expr { $$ = pub3::zone_inline_expr_t::alloc ($1); }
	|      p3_pub_zone       { $$ = $1; }
	;

/* ---------------------------------------------------------------------- */

p3_inline_expr: 
        T_P3_BEGIN_EXPR p3_expr '}' { $$ = $2; }
        ;

/*----------------------------------------------------------------------- */
/* Below follows the new support for generic sexpr's, which will appear
 * in {% cond %} statements and also filters.
 */

p3_pub_zone: T_P3_OPEN p3_pub_zone_inner T_P3_CLOSE
	{
	   $$ = $2;
        }
	;

p3_pub_zone_inner: p3_statement_opt p3_pub_zone_body_opt
       {
	 ptr<pub3::zone_pub_t> r = $2;
	 r->take_reserved_slot ($1);
	 $$ = r;
       }
       ;

p3_pub_zone_body_opt: /* empty */ { $$ = pub3::zone_pub_t::alloc (); }
	| p3_pub_zone_body        { $$ = $1; }
        ;

p3_pub_zone_body: p3_pub_zone_pair
       {
         ptr<pub3::zone_pub_t> r = pub3::zone_pub_t::alloc ();
	 r->add ($1);
	 $$ = r;
       } 
       | p3_pub_zone_body p3_pub_zone_pair
       {
         $$ = $1;
	 $$->add($2);
       }
       ;

p3_pub_zone_pair: p3_control p3_statement_opt
       {
         $$.first = $1;
	 $$.second = $2;
       }
       ;

p3_control:     p3_for { $$ = $1; }
	      | p3_if { $$ = $1; }
	      | p3_locals { $$ = $1; }
	      | p3_universals { $$ = $1; }
	      | p3_globals { $$ = $1; }
	      | p3_include { $$ = $1; }
	      | p3_print { $$ = $1; }
	      | p3_html_zone { $$ = pub3::statement_zone_t::alloc ($1); }
	      | p3_switch { $$ = $1; }
	      | p3_fndef { $$ = $1; }
	      | p3_break { $$ = $1; }
	      | p3_return { $$ = $1; }
	      | p3_continue { $$ = $1; }
	      | p3_while { $$ = $1; }
	      | p3_exit { $$ = $1; }
              | ';' { $$ = NULL; }
	      ;

p3_fndef : T_P3_DEF p3_identifier p3_fndef_body
	 {
	    $$ = pub3::fndef_t::alloc ($2, $3);
	 }
	 ;

p3_lambda: T_P3_LAMBDA p3_fndef_body { $$ = $2; } ;


p3_fndef_body: '(' p3_identifier_list_opt ')' p3_nested_zone
       {
          ptr<pub3::lambda_t> l = pub3::lambda_t::alloc ($2, $4);
	  $$ = l;
       }
       ;

p3_break : T_P3_BREAK
       { $$ = pub3::break_t::alloc (); } 
       ;

p3_continue : T_P3_CONTINUE
       { $$ = pub3::continue_t::alloc (); }
       ;

p3_exit : T_P3_EXIT
       { $$ = pub3::exit_t::alloc (); }
       ;

p3_return: T_P3_RETURN p3_return_value ';'
       { $$ = pub3::return_t::alloc ($2); } 
       ;

p3_return_value:                   { $$ = NULL; }
	         | p3_expr         { $$ = $1; }
		 ;

p3_switch : T_P3_SWITCH '(' p3_expr ')' '{' p3_switch_cases '}'
	  {
	     ptr<pub3::switch_t> s = pub3::switch_t::alloc ();
	     s->add_key ($3);
	     if (!s->add_cases ($6)) {
	     	  str err = "Got 2 or more default cases for switch()";
	     	  pub3::parse_error (err);
	     }
	     $$ = s;
	  }
	  ;

p3_switch_cases: /*empty */
          {
 	     ptr<pub3::case_list_t> l = pub3::case_list_t::alloc ();
	     $$ = l;
	  }
	  | p3_switch_cases p3_switch_case
	  {
	     $1->push_back ($2);
	     $$ = $1;
	  }
	  ;

p3_switch_case: T_P3_CASE p3_case_key_opt p3_nested_zone
          {
	     ptr<pub3::case_t> c = pub3::case_t::alloc ();
	     c->add_key ($2);
	     c->add_zone ($3);
	     $$ = c;
	  }
	  ;

p3_case_key_opt : '(' p3_constant_or_string ')' { $$ = $2; }
		| /* empty */		        { $$ = NULL; }
		;

p3_statement_opt: /*empty*/ { $$ = NULL; }
	      | p3_statement
	      ;

p3_statement: p3_expr_statement { $$ = $1; }
	      ;

p3_expr_statement: p3_expr
	      {
	         $$ = pub3::expr_statement_t::alloc ($1);
	      }
	      ;

p3_expr: p3_assignment_expr;

p3_assignment_expr: p3_conditional_expr { $$ = $1; }
	       | p3_unary_expr '=' p3_assignment_expr
	       {
                  $$ = pub3::expr_assignment_t::alloc ($1, $3);
	       }
	       ;

p3_conditional_expr: p3_logical_OR_expr;

p3_logical_OR_expr: p3_logical_AND_expr
	       {
	          $$ = $1;
	       }
	       | p3_logical_OR_expr T_P3_OR p3_logical_AND_expr
	       {
	          $$ = pub3::expr_OR_t::alloc ($1, $3);
               }
	       ;

p3_logical_AND_expr: p3_inclusive_OR_expr
               { 
	          $$ = $1 ; 
	       }
	       | p3_logical_AND_expr T_P3_AND p3_inclusive_OR_expr
               { 
	          $$ = pub3::expr_AND_t::alloc ($1, $3);
	       }
	       ;

p3_inclusive_OR_expr: p3_equality_expr
	       {
	          $$ = $1;
	       }
	       | p3_inclusive_OR_expr T_P3_PIPE p3_equality_expr
	       {
	          ptr<pub3::call_t> c = $3->coerce_to_call ();
 		  if (!c) {
		     str err = "Cannot coerce expression into a function call";
		     pub3::parse_error (err);
	             $$ = NULL;
                  } else {
                     c->unshift_argument ($1);
		     $$ = c;
		  }
	       }
	       ;

p3_equality_expr: p3_relational_expr
		{
  		   $$ = $1;
		}
	 	| p3_equality_expr p3_equality_op p3_relational_expr
		{
		   $$ = pub3::expr_EQ_t::alloc ($1, $3, $2);
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
	      $$ = pub3::expr_relation_t::alloc ($1, $3, $2);
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
	     $$ = pub3::expr_add_t::alloc ($1, $3, $2);
	   }
	   ;

p3_multiplicative_expr:
             p3_unary_expr
           {
	      $$ = $1;
	   }
	   | p3_multiplicative_expr '%' p3_unary_expr
	   {
	     $$ = pub3::expr_mod_t::alloc ($1, $3);
	   }
	   | p3_multiplicative_expr '*' p3_unary_expr
	   {
	     $$ = pub3::expr_mult_t::alloc ($1, $3);
	   }
	   | p3_multiplicative_expr '/' p3_unary_expr
	   {
             $$ = pub3::expr_div_t::alloc ($1, $3);
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
	      $$ = pub3::expr_NOT_t::alloc ($2);
           }
	   ;

p3_dictref:  p3_postfix_expr '.' p3_identifier
	   {
	      $$ = pub3::expr_dictref_t::alloc ($1, $3);
	   }
	   ;

p3_vecref: p3_postfix_expr '[' p3_expr ']'
	   {
	      $$ = pub3::expr_vecref_t::alloc ($1, $3);
	   }
	   ;

p3_fncall: p3_postfix_expr '(' p3_argument_expr_list_opt ')' 
	   {
	      /* Allocate a stub at first, which will be resolved 
	       * into the true function either at evaluation time
	       * (in the pub command line client) or upon conversion
	       * from XDR (in OKWS services)
	       */
	      $$ = pub3::call_t::alloc ($1, $3, false);
           } 
           | p3_postfix_expr T_P3_BCALL_OPEN p3_argument_expr_list_opt 
               T_P3_BCALL_CLOSE
	   {
	      $$ = pub3::call_t::alloc ($1, $3, true);
           }
	   ;

p3_varref: p3_identifier 
           { 
              /* See comment in pub3expr.h -- this identifier might be
	       * a function call in a pipeline; we just don't know yet!
	       */
	      $$ = pub3::expr_varref_t::alloc ($1);
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
	   | p3_lambda       { $$ = $1; }
	   | p3_null         { $$ = $1; }
	   | p3_heredoc      { $$ = $1; }
	   ;

p3_null : T_P3_NULL { $$ = pub3::expr_null_t::alloc (); }
	;

p3_heredoc :  p3_heredoc_html | p3_heredoc_pub;


p3_heredoc_html : T_P3_HTML_HEREDOC p3_html_zone_inner T_2R_BRACE
        {
	   $$ = pub3::heredoc_t::alloc ($2);
	}
	;

p3_heredoc_pub : T_P3_PUB_HEREDOC p3_pub_zone_inner '}'
        {
	   $$ = pub3::heredoc_t::alloc ($2);
	}
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

p3_identifier_list_opt: /*empty */
          { $$ = New refcounted<pub3::identifier_list_t> (); }
          | p3_identifier_list 
          { $$ = $1; }
	  ;

p3_identifier_list: p3_identifier
          { 
	      $$ = New refcounted<pub3::identifier_list_t> (); 
	      $$->push_back ($1);
          }
	  | p3_identifier_list ',' p3_identifier
	  {
	     $1->push_back ($3);
	     $$ = $1;
	  }
	  ;

p3_regex: T_P3_REGEX
	  {
	     str err;
	     ptr<rxx> x;
             x =  pub3::rxx_factory_t::compile ($1.regex, $1.opts, &err);
	     if (err) {
               pub3::parse_error (err);
	     } else {
	       $$ = pub3::expr_regex_t::alloc (x, $1.regex, $1.opts);
             }
	  }
	  ;

p3_argument_expr_list_opt:           
           { 
              $$ = pub3::expr_list_t::parse_alloc ();
           }
           | p3_argument_expr_list   { $$ = $1; }
           ;

p3_argument_expr_list: p3_expr
           {
	      $$ = pub3::expr_list_t::parse_alloc ();
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
	   | p3_floating_constant
	   {
	      $$ = New refcounted<pub3::expr_double_t> ($1); 
	   }
           | p3_boolean_constant
	   {  
	      $$ = pub3::expr_bool_t::alloc ($1);
	   }
	   ;

p3_constant_or_string: p3_integer_constant { $$ = $1; }
	   | p3_boolean_constant       { $$ = pub3::expr_bool_t::alloc ($1); }
	   | p3_string_constant            { $$ = $1; }
	   | p3_null		           { $$ = $1; }
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
	      pub3::parse_error (b);
	   }
	   $$ = New refcounted<pub3::expr_int_t> (i);
	}
	| T_P3_UINT
	{
	   u_int64_t u = 0;
	   if (!convertuint ($1, &u)) {
	      strbuf b ("Cannot convert '%s' to unsigned int", $1.cstr ());
	      pub3::parse_error (b);
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
	      pub3::parse_error (b);
           }
	   $$ = d;
	}
	;

p3_dictionary: '{' p3_bindings_opt '}' { $$ = $2; }
	       ;

p3_bindlist: '{' p3_bindlist_bindings '}' { $$ = $2; }

p3_bind_value_opt : /* empty */ { $$ = NULL; }
	| p3_bindchar p3_expr { $$ = $2; }
	;

p3_bindings_opt: 
         /* empty */  { $$ = pub3::expr_dict_t::parse_alloc (); }
	| p3_bindings { $$ = $1; }
	;

p3_bindlist_bindings: p3_binding 
        { 
	   $$ = pub3::bindlist_t::alloc (); 
	   $$->insert ($1);
        }
        | p3_bindlist_bindings ',' p3_binding
	{
	   $$ = $1;
	   $$->insert ($3);
	}
	;

p3_bindings: p3_binding
        {
	   ptr<pub3::expr_dict_t> d = pub3::expr_dict_t::parse_alloc ();
           d->insert ($1);
	   $$ = d;	 
	}
	| p3_bindings ',' p3_binding
	{
	   $1->insert ($3);
           $$ = $1;
	}
	;

p3_binding: p3_bind_key p3_bind_value_opt
	{
	   $$ = pub3::binding_t ($1, $2);
	}
	;

p3_bind_key: p3_identifier    { $$ = $1; }
        | p3_string_constant  { $$ = $1->to_str (); }
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
           $$ = pub3::expr_shell_str_t::alloc ();
	}
	| p3_string_elements
	{
	   $$ = $1;
	}
	;

p3_string_elements: 
          p3_string_element 
        { 
           $$ = pub3::expr_shell_str_t::alloc ();
	   $$->add ($1);
	}
        | p3_string_elements p3_string_element
	{
	   $$ = $1;
	   $$->add ($2);
	}
	;

p3_string_element: 
          T_P3_STRING { $$ = pub3::expr_str_t::alloc ($1); }
	| T_P3_CHAR 
	{ 
	   $$ = pub3::expr_str_t::alloc (strbuf ("%c", $1));
	}
	| p3_inline_expr 
        { 
           $$ = $1; 
        }
	;

p3_string_constant: '"' p3_string_constant_inner '"'   { $$ = $2; }
        |           '\'' p3_string_constant_inner '\'' { $$ = $2; }
	;

p3_string_constant_inner: /* empty */ { $$ = pub3::expr_strbuf_t::alloc (); }
        | p3_string_constant_inner p3_string_constant_element
	{
	   $1->add ($2);
	   $$ = $1;
	}
	;

p3_string_constant_element: T_P3_STRING { $$ = $1; }
        | T_P3_CHAR { $$ = strbuf ("%c", $1); }
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
	  ptr<pub3::expr_list_t> l = pub3::expr_list_t::parse_alloc ();
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
          T_P3_INCLUDE { $$ = pub3::include_t::alloc (); }
        | T_P3_LOAD    { $$ = pub3::load_t::alloc (); }
	;
		   
	
p3_include: p3_include_or_load p3_flexi_tuple 
        {
	   str err;
	   ptr<pub3::include_t> i = $1;
	   if (!i->add_args ($2, &err)) {
	      pub3::parser_t::current ()->error (err);
	   }
           $$ = i;
	};

p3_locals_arg: '(' p3_bindlist ')' { $$ = $2; }
	    | p3_bindlist { $$ = $1; }
	    ;

p3_t_universals : T_P3_UNIVERSALS { $$ = pub3::plineno (); } ;

p3_universals: p3_t_universals p3_locals_arg
	{
	   ptr<pub3::universals_t> u = pub3::universals_t::alloc ($1);
	   u->add ($2);
	   $$ = u;
        }
        ;

p3_t_globals: T_P3_GLOBALS { $$ = pub3::plineno (); } ;

p3_globals: p3_t_globals p3_locals_arg
	{
	   ptr<pub3::globals_t> u = pub3::globals_t::alloc ($1);
	   u->add ($2);
	   $$ = u;
        }
        ;

p3_t_locals : T_P3_LOCALS { $$ = pub3::plineno (); } ;
 
p3_locals: p3_t_locals p3_locals_arg
	{
	   ptr<pub3::locals_t> l = pub3::locals_t::alloc ($1);
           l->add ($2);   
	   $$ = l;
        }
	;

p3_nested_zone: p3_html_zone { $$ = $1; }
	| '{' p3_pub_zone_inner '}' { $$ = $2; }
	;

p3_t_for : T_P3_FOR { $$ = pub3::plineno (); } ;
p3_t_while : T_P3_WHILE { $$ = pub3::plineno (); } ;

p3_for: p3_t_for p3_flexi_tuple p3_nested_zone p3_empty_clause 
        {
	    ptr<pub3::for_t> f = pub3::for_t::alloc ($1);
	    if (!f->add_params ($2)) {
	       pub3::parse_error ("failed to set parameters for for() loop");
	    }
	    f->add_body ($3);
	    f->add_empty ($4);
	    $$ = f;
	};

p3_while : p3_t_while '(' p3_expr  ')' p3_nested_zone
        {
            ptr<pub3::while_t> w = pub3::while_t::alloc ($1);
            if (!w->add_cond ($3)) {
	       pub3::parse_error ("failed to set condition for while() loop");
	    }
	    w->add_body ($5);
	    $$ = w;
	};

p3_t_print: T_P3_PRINT { $$ = pub3::plineno (); } ;

p3_print: p3_t_print p3_flexi_tuple
       {
           ptr<pub3::print_t> p = pub3::print_t::alloc ($1);
	   if (!p->add ($2)) {
	     pub3::parse_error ("bad arguments passed to print");
	   }
	   $$ = p;
       }
       ;

p3_t_if : T_P3_IF { $$ = pub3::plineno (); } ;

p3_if: p3_t_if p3_if_clause p3_elifs_opt p3_else_opt
       {
          ptr<pub3::if_t> i = pub3::if_t::alloc ($1);
	  i->add_clause ($2);
	  i->add_clauses ($3);
	  i->add_clause ($4);
	  $$ = i;
       }
       ;

p3_elifs_opt: /*empty*/ { $$ = NULL; }
       | p3_elifs { $$ = $1; }
       ;

p3_else_opt: /* empty */ { $$ = NULL; }
       | p3_else { $$ = $1; }
       ;

p3_elifs: p3_elif 
       {
           $$ = New refcounted<pub3::if_clause_list_t> ();
	   $$->push_back ($1);
       }
       | p3_elifs p3_elif
       {
           $$ = $1;
	   $$->push_back ($2);
       }
       ;

p3_elif: T_P3_ELIF	p3_if_clause { $$ = $2; } ;

p3_else: T_P3_ELSE p3_nested_zone
       {
	   ptr<pub3::if_clause_t> c = pub3::if_clause_t::alloc ();
	   c->add_body ($2);
	   $$ = c;
       }
       ;

p3_if_clause: '(' p3_expr ')' p3_nested_zone
       {
	    ptr<pub3::if_clause_t> c = pub3::if_clause_t::alloc ();
	    c->add_expr ($2);
	    c->add_body ($4);
	    $$ = c;
       }
       ;

p3_empty_clause: 
         /* empty */                   { $$ = NULL; }
	 | T_P3_EMPTY p3_nested_zone   { $$ = $2; }
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
      { $$ = pub3::expr_dict_t::parse_alloc (); }
      | json_dict_pairs { $$ = $1; }
      ;

json_dict_pairs: 
        json_dict_pair 
      {
         ptr<pub3::expr_dict_t> d = pub3::expr_dict_t::parse_alloc ();
         d->insert ($1);
	 $$ = d;
      }
      | json_dict_pairs ',' json_dict_pair
      {
         $1->insert ($3);
	 $$ = $1;
      }
      ;

json_dict_pair: T_P3_STRING ':' json_obj
      {
         $$ = pub3::binding_t ($1, $3);
      }
      ;

json_null : T_P3_NULL { $$ = pub3::expr_null_t::alloc (); }
	  ;

json_list: '[' json_list_elems_opt ']' { $$ = $2; } ;

json_list_elems_opt: /* empty */ 
      {
         $$ = pub3::expr_list_t::parse_alloc ();
      }
      | json_list_elems { $$ = $1; }
      ;

json_list_elems: json_obj
      {
         $$ = pub3::expr_list_t::parse_alloc ();
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

