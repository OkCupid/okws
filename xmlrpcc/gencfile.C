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
  aout << "bool\n"
       << "xml_" << id << " (" << XML_OBJ << " *xml, void *objp)\n"
       << "{\n"
       << "  return xml_rpc_traverse (xml, *static_cast<" 
       << id << " *> (objp), NULL);\n"
       << "}\n"
       << "\n";
}

static void
dumpstruct (const rpc_sym *s)
{
  const rpc_struct *rs = s->sstruct.addr ();
  aout << "bool\n"
       << "rpc_traverse (" XML_OBJ " *t, " << rs->id << " &obj)\n"
       << "{\n";
  const rpc_decl *rd = rs->decls.base ();
  for ( ; rd < rs->decls.lim (); rd++) {
    aout << "  if (!xml_rpc_traverse (t, obj." << rd->id << ", \""
	 << rd->id << "\")) return false;\n";
  }
  aout << "  return true;\n"
       << "}\n\n";
}

void
pswitch (str prefix, const rpc_union *rs, str swarg,
	 void (*pt) (str, const rpc_union *rs, const rpc_utag *),
	 str suffix, void (*defac) (str, const rpc_union *rs))
{
  bool hasdefault = false;
  str subprefix = strbuf () << prefix << "  ";

  aout << prefix << "switch (" << swarg << ") {" << suffix;
  for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
    if (rt->swval) {
      if (rt->swval == "TRUE")
	aout << prefix << "case true:" << suffix;
      else if (rt->swval == "FALSE")
	aout << prefix << "case false:" << suffix;
      else
	aout << prefix << "case " << rt->swval << ":" << suffix;
    }
    else {
      hasdefault = true;
      aout << prefix << "default:" << suffix;
    }
    if (rt->tagvalid)
      pt (subprefix, rs, rt);
  }
  if (!hasdefault && defac) {
    aout << prefix << "default:" << suffix;
    defac (subprefix, rs);
  }
  aout << prefix << "}\n";
}

static void
punionmacro (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type == "void")
    aout << prefix << "res = true;\n";
  else
    aout << prefix << "res = xml_rpc_traverse (t, *obj."
	 << rt->tag.id << ", \"" << rt->tag.id << "\");\n";
  aout << prefix << "break;\n";
}

static void
punionmacrodefault (str prefix, const rpc_union *rs)
{
  aout << prefix << "res = true;\n";
  aout << prefix << "break;\n";
}

static void
dumpunion (const rpc_sym *s)
{
  const rpc_union *rs = s->sunion.addr ();
  aout << "bool\n"
       << "rpc_traverse (" XML_OBJ " *t, " << rs->id << " &obj)\n"
       << "{\n"
       << "  " << rs->tagtype << " tag = obj." << rs->tagid << ";\n"
       << "  if (!xml_rpc_traverse (t, tag, \"" << rs->tagid << "\"))\n"
       << "    return false;\n"
       << "  if (tag != obj." << rs->tagid << ")\n"
       << "    obj.set_" << rs->tagid << " (tag);\n\n"
       << "  bool res = true;\n";

  pswitch ("  ", rs, "tag", punionmacro, "\n", punionmacrodefault);
    
  aout << "  return res;\n"
       << "}\n";

  aout << "\n";
}

static void
dumpenum (const rpc_sym *s)
{
  const rpc_enum *rs = s->senum.addr ();
  aout << "bool rpc_traverse (" XML_OBJ " *t, " << rs->id << "&obj)\n"
       << "{\n"
       << "  int32_t val = obj;\n"
       << "  if (!rpc_traverse (t, val))\n"
       << "    return false;\n"
       << "  obj = " << rs->id << " (val);\n"
       << "  return true;\n"
       << "}\n";
}

vec<str> const_tab;
vec<str> prog_tab;

static void
populate_prog_tab (const str &s)
{
  prog_tab.push_back (s);
}

static void
mktbl (const rpc_program *rs)
{
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    str xdr_name = rpcprog (rs, rv);
    str name = strbuf ("xml_%s", xdr_name.cstr ());
    aout << "static const xml_rpcgen_table " << name << "_tbl[] = {\n"
	 << "  " << rs->id << "_" << rv->val << "_APPLY (XMLTBL_DECL)\n"
	 << "};\n"
	 << "const xml_rpc_program " << name << " = {\n"
	 << "  " << rs->id << ", " << rv->id << ", " 
	 << "&" << xdr_name << ", " 
	 << name << "_tbl,\n"
	 << "  sizeof (" << name << "_tbl" << ") / sizeof ("
	 << name << "_tbl[0]),\n"
	 << "  \"" << name << "\"\n"
	 << "};\n";
    populate_prog_tab (name);
  }
  aout << "\n";
}

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
    dumpstruct (s);
    break;
  case rpc_sym::UNION:
    mkmshl (s->sunion->id);
    dumpunion (s);
    break;
  case rpc_sym::ENUM:
    mkmshl (s->senum->id);
    populate_const_table_enum (s);
    dumpenum (s);
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
dump_const_table (str fname)
{
  aout << "xml_rpc_const_t " << fname << "_rpc_constants[] = {\n";
  for (size_t i = 0; i < const_tab.size (); i++) {
    aout << "  { \"" << const_tab[i] << "\", " << const_tab[i]  << " },\n";
  }
  aout << "  { NULL, 0 }\n";
  aout << "};\n\n";
}

static void
dump_prog_table (str fname)
{
  aout << "static const xml_rpc_program *" << fname << "_rpc_programs[] = {\n";
  for (size_t i = 0; i < prog_tab.size (); i++) {
    aout << "  &" << prog_tab[i] << ",\n";
  }
  aout << "  NULL\n"
       << "};\n\n";
}

static void
dump_file_struct (str prfx)
{
  aout << "xml_rpc_file " << prfx << "_rpc_file = {\n"
       << "  " << prfx << "_rpc_programs,\n"
       << "  " << prfx << "_rpc_constants,\n"
       << "  \"" << prfx << "\"\n"
       << "};\n\n";
}

void
gencfile (str fname, str xdr_headername)
{
  aout << "// -*-c++-*-\n"
       << "/* This file was automatically generated by rpcc. */\n\n"
       << "#include \"" << makehdrname (fname) << "\"\n\n";

  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++)
    dumpsym (s);

  str prfx = stripfname (fname, false);

  dump_const_table (prfx);
  dump_prog_table (prfx);

  dump_file_struct (prfx);

  aout << "\n";
}



