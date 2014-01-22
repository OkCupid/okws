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
#include "rxx.h"

static void collect_rpctype (str i);

static void
mkmshl_xml (str id)
{
  if (skip_xml) return;
  aout << "bool\n"
       << "xml_" << id << " (" << XML_OBJ << " *xml, void *objp)\n"
       << "{\n"
       << "  return xml_rpc_traverse (xml, *static_cast<" 
       << id << " *> (objp), NULL);\n"
       << "}\n"
       << "\n";
}

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
       << "    {\n"
       << "      ptr<v_XDR_t> v = xdr_virtualize (xdrs);\n"
       << "      if (v) {\n"
       << "        return rpc_traverse (v, *static_cast<"
       << id << " *> (objp));\n"
       << "      } else {\n"
       << "        return rpc_traverse (xdrs, *static_cast<"
       << id << " *> (objp));\n"
       << "      }\n"
       << "    }\n"
       << "    break;\n"
       << "  case XDR_FREE:\n"
       << "    rpc_destruct (static_cast<" << id << " *> (objp));\n"
       << "    return true;\n"
       << "  default:\n"
       << "    panic (\"invalid xdr operation %d\\n\", xdrs->x_op);\n"
       << "    return false;\n"
       << "  }\n"
       << "}\n"
       << "\n";
  
  mkmshl_xml (id);
  collect_rpctype (id);
}

vec<str> const_tab;
vec<str> prog_tab;
vec<str> type_tab;

static void
populate_prog_tab (const str &s)
{
  prog_tab.push_back (s);
}

static void
mktbl_xml (const rpc_program *rs)
{
  if (skip_xml) return;
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
mktblentry (u_int procno, const rpc_arg &arg, const rpc_arg &res) {
    aout << "  {\n"
         << "    \"" << procno << "\",\n"
         << "    &typeid (" << arg.type << "), " << arg.type << "_alloc, "
         << (arg.compressed
                 ? strbuf() << "snappy_xdr_arg<xdr_" << arg.type << ">"
                 : "xdr_" << arg.type
            )
         << ", print_" << arg.type << ",\n"
         << "    &typeid (" << res.type << "), " << res.type << "_alloc, "
         << (res.compressed
                 ? strbuf() << "snappy_xdr_arg<xdr_" << res.type << ">"
                 : "xdr_" << res.type
            )
         << ", print_" << res.type << ",\n"
         << "  }";
}

static void
mktbl (const rpc_program *rs)
{
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    str name = rpcprog (rs, rv);
    aout << "static const rpcgen_table " << name << "_tbl[] = {\n";
    u_int n{0};
    for (const rpc_proc &rp : rv->procs) {
        while (n++ < rp.val) {
            if (n != 1) { aout << ",\n"; }
            mktblentry (n-1, {"false"}, {"false"});
        }
        if (n != 1) { aout << ",\n"; }
        mktblentry (n-1, rp.arg, rp.res);
    }
    aout << "\n};\n"
	 << "const rpc_program " << name << " = {\n"
	 << "  " << rs->id << ", " << rv->id << ", " << name << "_tbl,\n"
	 << "  sizeof (" << name << "_tbl" << ") / sizeof ("
	 << name << "_tbl[0]),\n"
	 << "  \"" << name << "\"\n"
	 << "};\n";
  }
  aout << "\n";
  mktbl_xml (rs);
}

static void
dumpstructmember_xml(str swval, const rpc_decl *rd)
{
  if (swval)
    aout << "  { \"" << swval << "\", ";
  else
    aout << "  { NULL, ";
  if (rd->id)
    aout << "\"" << rd->id << "\", ";
  else
    aout << "NULL, ";
  if (rd->type)
    aout << "&xml_typeinfo_" << rd->type << ", ";
  else
    aout << "NULL, ";
  aout << (int)rd->qual << ", "
       << (rd->bound ? rd->bound : "0") << " }";
}

static void
dumpstruct_xml (const rpc_sym *s)
{
  if (skip_xml) return;
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

 aout << "static xml_struct_entry_t _xml_contents_" << rs->id << "[] = {\n";
  for (rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    dumpstructmember_xml(NULL, rd);
    aout << ",\n";
  }
  aout << "  { NULL, NULL, NULL, 0, 0 }\n";
  aout << "};\n\n";

  aout << "xml_typeinfo_t xml_typeinfo_" << rs->id << " = {\n"
       << "  \"" << rs->id << "\",\n"
       << "  xml_typeinfo_t::STRUCT,\n"
       << "  _xml_contents_" << rs->id << ",\n"
       << "};\n\n";
  type_tab.push_back(rs->id);
}

static void
punionmacro_xml (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type == "void")
    aout << prefix << "res = true;\n";
  else
    aout << prefix << "res = xml_rpc_traverse (t, *obj."
	 << rt->tag.id << ", \"" << rt->tag.id << "\");\n";
  aout << prefix << "break;\n";
}

static void
punionmacrodefault_xml (str prefix, const rpc_union *rs)
{
  aout << prefix << "res = true;\n";
  aout << prefix << "break;\n";
}

static void
dumpunion_xml (const rpc_sym *s)
{
  if (skip_xml) return;
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

  pswitch ("  ", rs, "tag", punionmacro_xml, "\n", punionmacrodefault_xml);
    
  aout << "  return res;\n"
       << "}\n\n";

  aout << "xml_struct_entry_t _xml_contents_" << rs->id << "[] = {\n"
       << "  { NULL, \"" << rs->tagid << "\", &xml_typeinfo_" 
       << rs->tagtype << ", 0, 0},\n";
  for (const rpc_utag *rc = rs->cases.base(); rc < rs->cases.lim(); rc++) {
    dumpstructmember_xml(rc->swval, &rc->tag);
    aout << ",\n";
  }
  aout << "  { NULL, NULL, NULL, 0, 0 }," 
       << "};\n\n";

  aout << "xml_typeinfo_t xml_typeinfo_" << rs->id << " = {\n"
       << "  \"" << rs->id << "\",\n"
       << "  xml_typeinfo_t::UNION,\n"
       << "  _xml_contents_" << rs->id << ",\n"
       << "};\n\n";
  type_tab.push_back(rs->id);
}

static void
dumpenum_xml (const rpc_sym *s)
{
  if (skip_xml) return;
  const rpc_enum *rs = s->senum.addr ();
  aout << "bool rpc_traverse (" XML_OBJ " *t, " << rs->id << "&obj)\n"
       << "{\n"
       << "  int32_t val = obj;\n"
       << "  if (!rpc_traverse (t, val))\n"
       << "    return false;\n"
       << "  obj = " << rs->id << " (val);\n"
       << "  return true;\n"
       << "}\n\n";

  aout << "xml_typeinfo_t xml_typeinfo_" << rs->id << " = {\n"
       << "  \"" << rs->id << "\",\n"
       << "  xml_typeinfo_t::ENUM,\n"
       << "  NULL\n"
       << "};\n\n";

  type_tab.push_back(rs->id);
}

//-----------------------------------------------------------------------
// MK 2010/11/1 --- mimic, for now, the behavior of rpcc, which has
// its own scheme for collecting RPC constants.  I'm hoping that eventually
// we can rip out all of xmlrpcc in favor of the newer, better 
// JSON rpc, so excuse the copy-paste from rpcc in sfslite for now...

struct rpc_constant_t {
  rpc_constant_t (str i, str t) : id (i), typ (t) {}
  str id, typ;
};

vec<rpc_constant_t> rpc_constants;
vec<str> rpc_types;

void collect_constant (str i, str t)
{
  rpc_constants.push_back (rpc_constant_t (i, t));
}

void collect_rpctype (str i)
{
  rpc_types.push_back (i);
}

str
make_csafe_filename (str fname)
{
  strbuf hdr;
  const char *fnp, *cp;

  if ((fnp = strrchr (fname.cstr(), '/')))
    fnp++;
  else fnp = fname;

  // strip off the suffix ".h" or ".C"
  for (cp = fnp; *cp && *cp != '.' ; cp++ ) ;
  size_t len = cp - fnp;

  mstr out (len + 1);
  for (size_t i = 0; i < len; i++) {
    if (fnp[i] == '-') { out[i] = '_'; }
    else { out[i] = fnp[i]; }
  }
  out[len] = 0;
  out.setlen (len);

  return out;
}

str 
make_constant_collect_hook (str fname)
{
  strbuf b;
  str csafe_fname = make_csafe_filename (fname);
  b << csafe_fname << "_constant_collect";
  return b;
}

static void
dump_constant_collect_hook (str fname)
{
  str cch = make_constant_collect_hook (fname);
  aout << "void\n"
       << cch << " (rpc_constant_collector_t *rcc)\n"
       << "{\n";
  for (size_t i = 0; i < rpc_constants.size (); i++) {
    const rpc_constant_t &rc = rpc_constants[i];
    aout << "  rcc->collect (\"" << rc.id << "\", "
	 << rc.id << ", " << rc.typ << ");\n";
  }
  for (size_t i = 0; i < rpc_types.size (); i++) {
    str id = rpc_types[i];
    aout << "  rcc->collect (\"" << id << "\", "
	 << "xdr_procpair_t (" << id << "_alloc, xdr_" << id << "));\n";
  }
  aout << "}\n\n";
}

//-----------------------------------------------------------------------

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
    collect_constant (rc->id, "RPC_CONSTANT_ENUM");
  }
}

static void
populate_const_table_prog (const rpc_program *rs)
{
  populate_const_table (rs->id);
  collect_constant (rs->id, "RPC_CONSTANT_PROG");
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    populate_const_table (rv->id);
    collect_constant (rv->id, "RPC_CONSTANT_VERS");
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      populate_const_table (rp->id);
      collect_constant (rp->id, "RPC_CONSTANT_PROC");
    }
  }
}

vec<str> pound_defs;

static void
collect_pound_def (str s)
{
  static rxx x ("#\\s*define\\s*(\\S+)\\s+(.*)");
  if (guess_defines && x.match (s)) {
    pound_defs.push_back (x[1]);
    collect_constant (x[1], "RPC_CONSTANT_POUND_DEF");
  }
}

static void
dump_pound_defs (str fn)
{
  aout << "static void\n"
       << fn << "_pound_defs_fn (xml_pound_def_collector_t *c)\n"
       << "{\n";
  for (size_t i = 0; i < pound_defs.size (); i++) {
    aout << "  c->collect (\"" << pound_defs[i] << "\", "
	 << pound_defs[i] << ");\n";
  }
  aout << "}\n\n";

}

static void
mkns (const rpc_namespace *ns)
{
  for (const rpc_program *rp = ns->progs.base (); rp < ns->progs.lim (); rp++) {
    mktbl (rp);
    populate_const_table_prog (rp);
  }
}

static void
dumptypedef_xml (const rpc_sym *s)
{
  if (skip_xml) return;
  const rpc_decl *rd = s->stypedef.addr ();
  aout << "static xml_struct_entry_t _xml_typedef_" << rd->id << " = \n";
  dumpstructmember_xml(NULL, rd);
  aout << ";\n\n";
  aout << "xml_typeinfo_t xml_typeinfo_" << rd->id << " = {\n"
       << "  \"" << rd->id << "\",\n"
       << "  xml_typeinfo_t::TYPEDEF,\n"
       << "  &_xml_typedef_" << rd->id << "\n"
       << "};\n\n";

  type_tab.push_back(rd->id);
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
    dumpstruct_xml (s);
    break;
  case rpc_sym::UNION:
    mkmshl (s->sunion->id);
    dumpunion_xml (s);
    break;
  case rpc_sym::ENUM:
    mkmshl (s->senum->id);
    dumpenum_xml (s);
    populate_const_table_enum (s);
    break;
  case rpc_sym::TYPEDEF:
    mkmshl (s->stypedef->id);
    dumptypedef_xml (s);
    break;
  case rpc_sym::PROGRAM:
    {
      const rpc_program *rp = s->sprogram.addr ();
      mktbl (rp);
      populate_const_table_prog (rp);
    }
    break;
  case rpc_sym::NAMESPACE:
    mkns (s->snamespace);
    break;
  case rpc_sym::LITERAL:
    collect_pound_def (*s->sliteral);
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
    "  const char *p;\n"
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
  const rpc_decl *dp = s->decls.base (), *ep = s->decls.lim ();
  size_t num = ep - dp;
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
    "  str npref;\n"
    "  if (prefix) {\n"
    "    npref = strbuf (\"%s  \", prefix);\n"
    "    sb << \"{\\n\";\n"
    "  }\n"
    "  else {\n"
    "    sb << \"{ \";\n"
    "  }\n";

  if (num > 1) {
    aout <<
      "  const char *sep = NULL;\n"
      "  if (prefix) {\n"
      "    sep = \"\";\n"
      "  } else {\n"
      "    sep = \", \";\n"
      "  }\n" ;
  }

  if (dp < ep)
    aout <<
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref.cstr());\n";
  while (++dp < ep)
    aout <<
      "  sb << sep;\n"
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref.cstr());\n";
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
      " recdepth, \"" << rt->tag.id << "\", npref.cstr());\n";
  aout << prefix << "break;\n";
}

static void
print_break (str prefix, const rpc_union *rs)
{
  aout << prefix << "break;\n";
}

static bool will_need_sep(const rpc_union* rs) {
  for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
    if (rt->tag.type != "void")
        return true;
  }
  return false;
}

static void
print_union (const rpc_union *s)
{
  bool ns = will_need_sep(s);
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
    << ((ns) ? "  const char *sep;\n" : "") <<
    "  str npref;\n"
    "  if (prefix) {\n"
    "    npref = strbuf (\"%s  \", prefix);\n" 
    << ((ns) ? "    sep = \"\";\n" : "") <<
    "    sb << \"{\\n\";\n"
    "  }\n"
    "  else {\n"
    << ((ns) ? "    sep = \", \";\n" : "") <<
    "    sb << \"{ \";\n"
    "  }\n"
    "  rpc_print (sb, obj." << s->tagid << ", recdepth, "
    "\"" << s->tagid << "\", npref.cstr());\n";
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

  if ((p = strrchr (fname.cstr(), '/')))
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
dump_type_table (str fname)
{
  aout << "static const xml_typeinfo_t *" << fname << "_rpc_types[] = {\n";
  for (size_t i = 0; i < type_tab.size (); i++) {
    aout << "  &xml_typeinfo_" << type_tab[i] << ",\n";
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
       << "  " << prfx << "_rpc_types,\n"
       << "  \"" << prfx << "\",\n"
       << "  " << prfx << "_pound_defs_fn\n"
       << "};\n\n";
}

void
gencfile (str fname)
{
  aout << "// -*-c++-*-\n"
       << "/* This file was automatically generated by xmlrpcc. */\n\n";
  if  (!skip_xml) {
    aout << "#define ENABLE_XML_XDR 1\n";
  }
  aout << "#include \"" << makehdrname (fname) << "\"\n"
       << "#include \"xdrsnappy.h\"\n\n";

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

  aout << "\n";

  str prfx = stripfname (fname, false);

  if (!skip_xml) {
    dump_const_table (prfx);
    dump_prog_table (prfx);
    dump_type_table (prfx);
    dump_pound_defs (prfx);
    dump_file_struct (prfx);
  }

  dump_constant_collect_hook (fname);

  aout << "\n";
}

