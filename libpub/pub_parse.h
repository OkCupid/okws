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
#include "parr.h"
#include "pub3.h"

extern int yy_wss_nl;      /* 1 over the fraction of \n to leave in */
extern int yyesc;          /* on if we're to gobble up espaces in strings */

/* scan.ll */
int yyerror (str s = NULL);
int yywarn (str s);
void yy_push_pubstate (pfile_type_t t);
void yy_pop_pubstate ();
void yyswitch (yy_buffer_state *s);
yy_buffer_state *yycreatebuf (FILE *fp);
extern int yyparse ();
extern int yylex ();
void scanner_reset (void);
void yy_parse_json (str s);
int yy_get_json_lineno ();
void yy_parse_fail();

#ifdef PDEBUG
extern int yydebug;
#endif

struct yystype_regex_pair_t {
  str regex;
  str opts;
};

struct yystype {
  char ch;
  int64_t num;
  ::str str;
  yystype_regex_pair_t regex;
  pub3::expr_statement_t *p3es;

  ptr<pub3::cond_clause_list_t> p3cclist;
  ptr<pub3::cond_clause_t> p3cc;
  ptr<pub3::expr_t> p3expr;
  ptr<pub3::expr_list_t> p3exprlist;
  ptr<pub3::expr_shell_str_t> p3str;
  ptr<pub3::expr_dict_t> p3dict;
  pub3::zone_pub_t::pair_t p3pair;
  bool bl;
  xpub3_relop_t relop;
  double dbl;
  pub3::include_t *p3include;
  pub3::print_t *print;
  ptr<pub3::zone_t> p3zone;
  ptr<pub3::statement_t> p3statement;
  ptr<pub3::case_t> p3case;
  ptr<pub3::case_list_t> p3cl;
  pub3::binding_t p3bind;

};

#define YYSTYPE yystype

extern YYSTYPE yylval;

#endif /* _LIBPUB_PUBPARSE_H */
