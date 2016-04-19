// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _LIBPUB_PUBPARSE_H
#define _LIBPUB_PUBPARSE_H 1

#include "pub.h"
#include "pubutil.h"
#include "pub3.h"

extern int yy_wss_nl;      /* 1 over the fraction of \n to leave in */
extern int yyesc;          /* on if we're to gobble up espaces in strings */

/* scan.ll */
struct yy_buffer_state;
yy_buffer_state *yy_create_buffer (FILE *ip, int sz);
void yy_switch_to_buffer (yy_buffer_state *s);
void yy_delete_buffer (yy_buffer_state *s);
void yy_pop_all ();
int yyerror (str s = NULL);


extern int yyparse ();
extern int yylex ();
void scanner_reset (void);
void yy_parse_json (str s);
void yy_parse_pub (str s);
void scanner_terminate (void);
void yy_push_html_state ();

#ifdef PDEBUG
extern int yydebug;
#endif

struct yystype_regex_pair_t {
  str regex;
  str opts;
};

struct yystype {
  char ch;
  ::str str;
  yystype_regex_pair_t regex;

  ptr<pub3::if_clause_list_t> p3iclist;
  ptr<pub3::if_clause_t> p3ic;
  ptr<pub3::expr_t> p3expr;
  ptr<pub3::expr_list_t> p3exprlist;
  ptr<pub3::expr_shell_str_t> p3str;
  ptr<pub3::expr_dict_t> p3dict;
  ptr<pub3::expr_strbuf_t> p3strbuf;
  pub3::zone_pub_t::pair_t p3pair;
  bool bl;
  xpub3_relop_t relop;
  double dbl;
  ptr<pub3::zone_t> p3zone;
  ptr<pub3::zone_pub_t> p3zp;
  ptr<pub3::statement_t> p3statement;
  ptr<pub3::case_t> p3case;
  ptr<pub3::case_list_t> p3cl;
  pub3::binding_t p3bind;
  ptr<pub3::identifier_list_t> p3strv;
  ptr<pub3::bindlist_t> p3bl;
  ptr<pub3::include_t> p3include;
  ptr<pub3::lambda_t> p3lambda;
  int64_t num;
  int lineno;
};

#define YYSTYPE yystype

extern YYSTYPE yylval;

#endif /* _LIBPUB_PUBPARSE_H */
