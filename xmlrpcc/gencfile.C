/* $Id: gencfile.C 912 2005-06-21 16:37:07Z max $ */

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

#include "rpcc.h"

static void
mkmshl (str id)
{
#if 0
  if (!needtype[id])
    return;
#endif
  aout << "void *\n"
       << id << "_alloc ()\n"
       << "{\n"
       << "  return New " << id << ";\n"
       << "}\n"
#if 0
       << "void\n"
       << id << "_free (void *objp)\n"
       << "{\n"
       << "  delete static_cast<" << id << " *> (objp);\n"
       << "}\n"
#endif
       << XDR_RETURN "\n"
       << "xdr_" << id << " (XDR *xdrs, void *objp)\n"
       << "{\n"
       << "  switch (xdrs->x_op) {\n"
       << "  case XDR_ENCODE:\n"
       << "  case XDR_DECODE:\n"
       << "    return rpc_traverse (xdrs, *static_cast<"
       << id << " *> (objp));\n"
       << "  case XDR_FREE:\n"
       << "    rpc_destruct (static_cast<" << id << " *> (objp));\n"
       << "    return true;\n"
       << "  default:\n"
       << "    panic (\"invalid xdr operation %d\\n\", xdrs->x_op);\n"
       << "  }\n"
       << "}\n"
       << "\n"
       << XDR_RETURN "\n"
       << "xml_" << id << " (" << XML_BASE_OBJ << "*xml, void *objp)\n"
       << "{\n"
       << "  switch (xml->op()) {\n"
       << "  case XML_ENCODE:\n"
       << "  case XML_DECODE:\n"
       << "    return rpc_traverse (xml,*static_cast<" 
       << id << " *> (objp));\n"
       << "  case XML_FREE:\n"
       << "    rpc_destruct (static_cast<" << id << " *> (objp));\n"
       << "    return true;\n"
       << "  default:\n"
       << "    panic (\"invalid xdr operation %d\\n\", xml->op ());\n"
       << "  }\n"
       << "}\n"
       << "\n";
}

static void
mktbl (const rpc_program *rs)
{
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    str name = rpcprog (rs, rv);
    aout << "static const rpcgen_table " << name << "_tbl[] = {\n"
	 << "  " << rs->id << "_" << rv->val << "_APPLY (XDRTBL_DECL)\n"
	 << "};\n"
	 << "const rpc_program " << name << " = {\n"
	 << "  " << rs->id << ", " << rv->id << ", " << name << "_tbl,\n"
	 << "  sizeof (" << name << "_tbl" << ") / sizeof ("
	 << name << "_tbl[0]),\n"
	 << "  \"" << name << "\"\n"
	 << "};\n";
  }
  aout << "\n";
}

vec<str> const_tab;

static void
populate_const_table (const str &s)
{
  const_tab.push_back (s);
}

static void
populate_const_table_enum (const rpc_sym *s)
{
  const rpc_enum *rs = s->senum.addr ();
  for (const rpc_const *rc = rs->tags.base (); rc < rs->tags.lim (); rc++) {
    populate_const_table (rc->id);
  }
}

static void
populate_const_table_prog (const rpc_sym *s)
{
  const rpc_program *rs = s->sprogram.addr ();
  populate_const_table (rs->id);
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    populate_const_table (rv->id);
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      populate_const_table (rp->id);
    }
  }
}

static void
dumpsym (const rpc_sym *s)
{
  switch (s->type) {
  case rpc_sym::CONST:
    populate_const_table (s->sconst->id);
    break;
  case rpc_sym::STRUCT:
    mkmshl (s->sstruct->id);
    break;
  case rpc_sym::UNION:
    mkmshl (s->sunion->id);
    break;
  case rpc_sym::ENUM:
    mkmshl (s->senum->id);
    populate_const_table_enum (s);
    break;
  case rpc_sym::TYPEDEF:
    mkmshl (s->stypedef->id);
    break;
  case rpc_sym::PROGRAM:
    mktbl (s->sprogram.addr ());
    populate_const_table_prog (s);
    break;
  default:
    break;
  }
}

static void
print_print (str type)
{
  str pref (strbuf ("%*s", int (8 + type.len ()), ""));
  aout << "void\n"
    "print_" << type << " (const void *_objp, const strbuf *_sbp, "
    "int _recdepth,\n" <<
    pref << "const char *_name, const char *_prefix)\n"
    "{\n"
    "  rpc_print (_sbp ? *_sbp : warnx, *static_cast<const " << type
       << " *> (_objp),\n"
    "             _recdepth, _name, _prefix);\n"
    "}\n";

  aout << "void\n"
    "dump_" << type << " (const " << type << " *objp)\n"
    "{\n"
    "  rpc_print (warnx, *objp);\n"
    "}\n\n";
}

static void
print_enum (const rpc_enum *s)
{
  aout <<
    "const strbuf &\n"
    "rpc_print (const strbuf &sb, const " << s->id << " &obj, "
    "int recdepth,\n"
    "           const char *name, const char *prefix)\n"
    "{\n"
    "  char *p;\n"
    "  switch (obj) {\n";
  for (const rpc_const *cp = s->tags.base (),
	 *ep = s->tags.lim (); cp < ep; cp++)
    aout <<
      "  case " << cp->id << ":\n"
      "    p = \"" << cp->id << "\";\n"
      "    break;\n";
  aout <<
    "  default:\n"
    "    p = NULL;\n"
    "    break;\n"
    "  }\n"
    "  if (name) {\n"
    "    if (prefix)\n"
    "      sb << prefix;\n"
    "    sb << \"" << s->id << " \" << name << \" = \";\n"
    "  };\n"
    "  if (p)\n"
    "    sb << p;\n"
    "  else\n"
    "    sb << int (obj);\n"
    "  if (prefix)\n"
    "    sb << \";\\n\";\n"
    "  return sb;\n"
    "};\n";
  print_print (s->id);
}

static void
print_struct (const rpc_struct *s)
{
  aout <<
    "const strbuf &\n"
    "rpc_print (const strbuf &sb, const " << s->id << " &obj, "
    "int recdepth,\n"
    "           const char *name, const char *prefix)\n"
    "{\n"
    "  if (name) {\n"
    "    if (prefix)\n"
    "      sb << prefix;\n"
    "    sb << \"" << s->id << " \" << name << \" = \";\n"
    "  };\n"
    "  const char *sep;\n"
    "  str npref;\n"
    "  if (prefix) {\n"
    "    npref = strbuf (\"%s  \", prefix);\n"
    "    sep = \"\";\n"
    "    sb << \"{\\n\";\n"
    "  }\n"
    "  else {\n"
    "    sep = \", \";\n"
    "    sb << \"{ \";\n"
    "  }\n";
  const rpc_decl *dp = s->decls.base (), *ep = s->decls.lim ();
  if (dp < ep)
    aout <<
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref);\n";
  while (++dp < ep)
    aout <<
      "  sb << sep;\n"
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref);\n";
  aout <<
    "  if (prefix)\n"
    "    sb << prefix << \"};\\n\";\n"
    "  else\n"
    "    sb << \" }\";\n"
    "  return sb;\n"
    "}\n";
  print_print (s->id);
}

static void
print_case (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type != "void")
    aout
      << prefix << "sb << sep;\n"
      << prefix << "rpc_print (sb, *obj." << rt->tag.id << ", "
      " recdepth, \"" << rt->tag.id << "\", npref);\n";
  aout << prefix << "break;\n";
}

static void
print_break (str prefix, const rpc_union *rs)
{
  aout << prefix << "break;\n";
}

static void
print_union (const rpc_union *s)
{
  aout <<
    "const strbuf &\n"
    "rpc_print (const strbuf &sb, const " << s->id << " &obj, "
    "int recdepth,\n"
    "           const char *name, const char *prefix)\n"
    "{\n"
    "  if (name) {\n"
    "    if (prefix)\n"
    "      sb << prefix;\n"
    "    sb << \"" << s->id << " \" << name << \" = \";\n"
    "  };\n"
    "  const char *sep;\n"
    "  str npref;\n"
    "  if (prefix) {\n"
    "    npref = strbuf (\"%s  \", prefix);\n"
    "    sep = \"\";\n"
    "    sb << \"{\\n\";\n"
    "  }\n"
    "  else {\n"
    "    sep = \", \";\n"
    "    sb << \"{ \";\n"
    "  }\n"
    "  rpc_print (sb, obj." << s->tagid << ", recdepth, "
    "\"" << s->tagid << "\", npref);\n";
  pswitch ("  ", s, "obj." << s->tagid, print_case, "\n", print_break);
  aout <<
    "  if (prefix)\n"
    "    sb << prefix << \"};\\n\";\n"
    "  else\n"
    "    sb << \" }\";\n"
    "  return sb;\n"
    "}\n";
  print_print (s->id);
}

static void
dumpprint (const rpc_sym *s)
{
  switch (s->type) {
  case rpc_sym::STRUCT:
    print_struct (s->sstruct.addr ());
    break;
  case rpc_sym::UNION:
    print_union (s->sunion.addr ());
    break;
  case rpc_sym::ENUM:
    print_enum (s->senum.addr ());
    break;
  case rpc_sym::TYPEDEF:
    print_print (s->stypedef->id);
  default:
    break;
  }
}

static str
makehdrname (str fname)
{
  strbuf hdr;
  const char *p;

  if ((p = strrchr (fname, '/')))
    p++;
  else p = fname;

  hdr.buf (p, strlen (p) - 1);
  hdr.cat ("h");

  return hdr;
}

static void
dump_const_table ()
{
  aout << "static qhash<str, int> const_tab;\n";
  aout << "void init_const_tab ()\n"
       << "{\n";
  for (size_t i = 0; i < const_tab.size (); i++) {
    aout << "  const_tab.insert (\"" << const_tab[i] << "\","
	 << const_tab[i] << ")\n";
  }
  aout << "}\n\n";
}

void
gencfile (str fname)
{
  aout << "// -*-c++-*-\n"
       << "/* This file was automatically generated by rpcc. */\n\n"
       << "#include \"" << makehdrname (fname) << "\"\n\n";

#if 0
  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++)
    if (s->type == rpc_sym::PROGRAM)
      for (const rpc_vers *rv = s->sprogram->vers.base ();
	   rv < s->sprogram->vers.lim (); rv++)
	for (const rpc_proc *rp = rv->procs.base ();
	     rp < rv->procs.lim (); rp++) {
	  needtype.insert (rp->arg);
	  needtype.insert (rp->res);
	}
#endif

  aout << "#ifdef MAINTAINER\n\n";
  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++)
    dumpprint (s);
  aout << "#endif /* MAINTAINER*/\n";

  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++)
    dumpsym (s);

  dump_const_table ();

  aout << "\n";
}

