// -*-c++-*-
/* $Id: rpcc.h 1201 2005-11-28 19:20:01Z max $ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
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


/*
 * netdb.h hack
 *
 * There is a conflict betwen flex version 2.5.4 and /usr/include/netdb.h
 * on Linux. flex generates code that #define's __unused to be empty, but
 * in struct gaicb within netdb.h, there is a member field of a struct
 * called __unused, which gets blanked out, causing a compile error.
 * (Note that netdb.h is included from sysconf.h).  Use this hack to
 * not include netdb.h for now...
 */
#ifndef _NETDB_H
# define _SKIP_NETDB_H
# define _NETDB_H
#endif

#include "amisc.h"

#ifdef _SKIP_NETDB_H
# undef _NETDB_H
# undef _SKIP_NETDB_H
#endif
/*
 * end netdb.h hack
 */

#include "vec.h"
#include "union.h"
#include "qhash.h"

#if 1
#include "aios.h"
#else

#include <iostream.h>
inline ostream &
operator<< (ostream &os, const str &s)
{
  return os.write (s.cstr (), s.len ());
}
#define aout cout

#endif

inline const strbuf &
strbuf_cat (const strbuf &sb, char c)
{
  suio_copy (sb.tosuio (), &c, 1);
  return sb;
}

extern str idprefix;
extern bhash<str> ids;

#define XDR_RETURN "bool_t"	// XXX - should be bool

extern int lineno;
extern int printlit;
#undef yyerror
extern int yyerror (str);
#define yyerror yyerror		// For some versions of yacc
int yywarn (str);

extern int yylex ();
extern int yyparse ();
extern void checkliterals ();

struct rpc_decl {
  str id;
  str type;
  enum { SCALAR, PTR, ARRAY, VEC } qual;
  str bound;
};

struct rpc_const {
  str id;
  str val;
};

struct rpc_struct {
  str id;
  vec<rpc_decl> decls;
};

struct rpc_enum {
  str id;
  vec<rpc_const> tags;
};

struct rpc_utag {
  rpc_decl tag;
  str swval;
  bool tagvalid;
};

struct rpc_union {
  str id;
  str tagtype;
  str tagid;
  vec<rpc_utag> cases;
};

struct rpc_arg {
    str type;
    bool compressed;
};

struct rpc_proc {
  str id;
  u_int32_t val;
  rpc_arg arg;
  rpc_arg res;
};

struct rpc_vers {
  str id;
  u_int32_t val;
  vec<rpc_proc> procs;
};

struct rpc_program {
  str id;
  u_int32_t val;
  vec<rpc_vers> vers;
};

struct rpc_namespace {
  str id;
  vec<rpc_program> progs;
};

struct rpc_sym {
  union {
    union_entry_base _base;
    union_entry<rpc_const> sconst;
    union_entry<rpc_decl> stypedef;
    union_entry<rpc_struct> sstruct;
    union_entry<rpc_enum> senum;
    union_entry<rpc_union> sunion;
    union_entry<rpc_program> sprogram;
    union_entry<str> sliteral;
    union_entry<rpc_namespace> snamespace;
  };

  enum symtype { CONST, STRUCT, UNION, ENUM, TYPEDEF, PROGRAM, LITERAL,
		 NAMESPACE } type;

  rpc_sym () { _base.init (); }
  rpc_sym (const rpc_sym &s) : type (s.type) { _base.init (s._base); }
  ~rpc_sym () { _base.destroy (); }
private:
  rpc_sym &operator= (const rpc_sym &n)
    { type = n.type; _base.assign (n._base); return *this; }
public:

  symtype gettype () const { return type; }

  void settype (symtype t) {
    switch (type = t) {
    case NAMESPACE:
      snamespace.select ();
      break;
    case CONST:
      sconst.select ();
      break;
    case STRUCT:
      sstruct.select ();
      break;
    case UNION:
      sunion.select ();
      break;
    case ENUM:
      senum.select ();
      break;
    case TYPEDEF:
      stypedef.select ();
      break;
    case PROGRAM:
      sprogram.select ();
      break;
    case LITERAL:
      sliteral.select ();
      break;
    }
  }
};

struct YYSTYPE {
  u_int32_t num;
  struct rpc_arg arg;
  struct rpc_decl decl;
  struct rpc_const cnst;
  ::str str;
};
extern YYSTYPE yylval;

typedef vec<rpc_sym> symlist_t;
extern symlist_t symlist;

typedef vec<str> strlist_t;
extern strlist_t litq;

str rpcprog (const rpc_program *, const rpc_vers *);
void genheader (str);
void gencfile (str);

void pswitch (str prefix, const rpc_union *rs, str swarg,
	      void (*pt) (str, const rpc_union *rs, const rpc_utag *),
	      str suffix = "\n",
	      void (*defac) (str, const rpc_union *rs) = NULL);

#define XML_OBJ "XML_RPC_obj_t"

extern bool guess_defines;
extern bool skip_xml;

str stripfname (str s, bool suffix = true);
  
rpc_program *get_prog (bool creat);

str make_csafe_filename (str fname);
str make_constant_collect_hook (str fname);
