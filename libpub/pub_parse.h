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

#ifdef PDEBUG
extern int yydebug;
#endif

struct yystype {
  ptr<pval_t> pval;
  ptr<arg_t> arg;
  ptr<pvar_t> pvar;
  ptr<pstr_t> pstr;
  ptr<parr_t> parr;
  pfile_el_t *el;
  pfile_sec_t *sec;
  pfile_func_t *func;
  char ch;
  int64_t num;
  ::str str;
};

#define YYSTYPE yystype

extern YYSTYPE yylval;

#endif /* _LIBPUB_PUBPARSE_H */
