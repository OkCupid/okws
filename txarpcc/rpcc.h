// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
 *   updated for OKWS in 2003-4 by Maxwell Krohn (max@okcupid.com)
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

#include "amisc.h"
#include "vec.h"
#include "union.h"
#include "qhash.h"
#include "ptxa.h"
#include "crypt.h"
#include "callback.h"

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

struct ptxa_tok_t {
  ptxa_tok_t () {}
  ptxa_tok_t (const ptxa_tok_t &in) : id (in.id), sign (in.sign) {}
  const ptxa_tok_t & operator= (const ptxa_tok_t &in) 
   { id = in.id; sign = in.sign; return *this; }
  ptxa_tok_t (const str &i, bool s = true) : id (i), sign (s) {}
  str id;
  bool sign;
};

struct ptxa_tokset_t {
  ptxa_tokset_t () : positive_toks (false) {}
  inline void insert (const ptxa_tok_t &t) 
  { 
    tokens.insert (t.id, t.sign); 
    if (t.sign) positive_toks = true;
  }
  inline bool has_positive_toks () const { return positive_toks; }

  inline int access (const str &s) const
  {
    const bool *b = tokens[s];
    if (!b) return 0;
    else if (*b) return 1;
    else return -1;
  }

  void trav (callback<void, const str &, bool *>::ref cb) const;
  void dump (const str &p) const;

  qhash<str, bool> tokens;
  bool positive_toks;
};

struct rpc_txa_base_t {
  rpc_txa_base_t () {}
  virtual ~rpc_txa_base_t () {}

  str postpend (const str &prfx) const 
  { return strbuf (prfx) << "[" << id << "]";}

  bool has_positive_toks () const 
  { return tokset && tokset->has_positive_toks (); }
  ptr<ptxa_tokset_t> tokset;

  str id;
  u_int32_t val;
};

struct rpc_proc : public rpc_txa_base_t {
  rpc_proc () : rpc_txa_base_t () {}
  void dump (const str &prfx) const;
  str arg;
  str res;
};

struct rpc_program_p;
struct rpc_vers : public rpc_txa_base_t {
  rpc_vers () : rpc_txa_base_t () {}
  void dump (const rpc_program_p *p) const;
  bool has_positive_toks () const;
  vec<rpc_proc> procs;
  u_int32_t max_proc_id;
  str txa_rpc;
};


//
// rpc_program_p -- parse time representation of an RPC program;
// elsewhere in SFS there is an rpc_program type, which has runtime
// importance..
//
struct rpc_program_p : public rpc_txa_base_t {
  rpc_program_p () : rpc_txa_base_t () {}
  void dump () const;
  vec<rpc_vers> vers;
};

struct rpc_sym {
  union {
    union_entry_base _base;
    union_entry<rpc_const> sconst;
    union_entry<rpc_decl> stypedef;
    union_entry<rpc_struct> sstruct;
    union_entry<rpc_enum> senum;
    union_entry<rpc_union> sunion;
    union_entry<rpc_program_p> sprogram;
    union_entry<str> sliteral;
  };

  enum symtype { CONST, STRUCT, UNION, ENUM, TYPEDEF, PROGRAM, LITERAL } type;

  rpc_sym () { _base.init (); }
  rpc_sym (const rpc_sym &s) : type (s.type) { _base.init (s._base); }
  ~rpc_sym () { _base.destroy (); }
private:
  rpc_sym &operator= (const rpc_sym &n)
    { type = n.type; _base.assign (n._base); return *this; }
public:

  void settype (symtype t) {
    switch (type = t) {
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
  struct rpc_decl decl;
  struct rpc_const cnst;
  ::str str;
  ptxa_tok_t tok;
  ptr<ptxa_tokset_t> tokset;
};
extern YYSTYPE yylval;

typedef vec<rpc_sym> symlist_t;
extern symlist_t symlist;

// TXA stuff -- this sucks that it needs so many global variables;
// oh well; wanted to get it done sooner rather than later.
extern ptr<ptxa_tokset_t> ptxa_prog_toklist;
extern ptr<ptxa_tokset_t> ptxa_vers_toklist;
extern ptr<ptxa_tokset_t> ptxa_proc_toklist;
extern bhash<str> alltoks;
extern bool do_rnd_kbd;
extern str ptxa_seed;

typedef vec<str> strlist_t;
extern strlist_t litq;

str rpcprog (const rpc_program_p *, const rpc_vers *);
void genheader (str);
void gencfile (str);
void genpython (str);

void pswitch (str prefix, const rpc_union *rs, str swarg,
	      void (*pt) (str, const rpc_union *rs, const rpc_utag *),
	      str suffix = "\n",
	      void (*defac) (str, const rpc_union *rs) = NULL);
