/* -*-fundamental-*- */
/* $Id: scan.ll 912 2005-06-21 16:37:07Z max $ */

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

%option prefix="rpcc"
%{
#include "rpcc.h"
using namespace xmlrpcc;
#include "parse.h"

extern "C" int rpccwrap() { return 1; }

str filename = "(stdin)";
int lineno;
strlist_t litq;
%}

ID	[a-zA-Z_][a-zA-Z_0-9]*
WSPACE	[ \t]

%x GFILE GNL

%%
\n		++lineno;
{WSPACE}+	/* discard */;
^%.*		litq.push_back (rpcctext + 1);

^#pragma\ 	{ BEGIN (GNL); }
^#\ *[0-9]+\ *	{ lineno = atoi (rpcctext + 1); BEGIN (GFILE); }
<GFILE>.*	{ filename = str (rpcctext+1, rpccleng-2); BEGIN (GNL); }
<GFILE>\n	{ filename = "(stdin)"; BEGIN (GNL); }
<GNL>.		/* discard */;
<GNL>\n		BEGIN (0);

const		return T_CONST;
struct		return T_STRUCT;
union		return T_UNION;
enum		return T_ENUM;
typedef		return T_TYPEDEF;
program		return T_PROGRAM;
namespace       return T_NAMESPACE;

unsigned	return T_UNSIGNED;
int		return T_INT;
hyper		return T_HYPER;
double		return T_DOUBLE;
quadruple	return T_QUADRUPLE;
void		{ rpcclval.str = rpcctext; return T_VOID; }

version		return T_VERSION;
switch		return T_SWITCH;
case		return T_CASE;
default		return T_DEFAULT;

compressed	return T_COMPRESSED;

opaque		{ rpcclval.str = rpcctext; return T_OPAQUE; }
string		{ rpcclval.str = rpcctext; return T_STRING; }

array		|
bytes		|
destroy		|
free		|
getpos		|
inline		|
pointer		|
reference	|
setpos		|
sizeof		|
vector		{ rpccerror (strbuf ("illegal use of reserved word '%s'",
					rpcctext));
		}

{ID}		{ rpcclval.str = rpcctext; return T_ID; }
[+-]?[0-9]+	|
[+-]?0x[0-9a-fA-F]+	{ rpcclval.str = rpcctext; return T_NUM; }

[=;{}<>\[\]*,:()] return rpcctext[0];

[^ \t\n0-9a-zA-Z_=;{}<>\[\]*,:()][^ \t\n0-9a-zA-Z_]*	|
[0-9]*		{ rpccerror (strbuf ("syntax error at '%s'", rpcctext)); }
%%

/* .		rpccerror (strbuf ("syntax error at '%s'", rpcctext)); */
/* <<EOF>>		{ checkliterals (); return 0; } */

static void
xxx_gcc_bug_exit (int code)	/* XXX - work around bug in gcc on alpha */
{
  exit (code);
}

int
rpccerror (str msg)
{
  warnx << filename << ":" << lineno << ": " << msg << "\n";
  xxx_gcc_bug_exit (1);
  return 1;
}

int
rpccwarn (str msg)
{
  warnx << filename << ":" << lineno << ": Warning: " << msg << "\n";
  return 0;
}
