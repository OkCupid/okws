/* $Id: genheader.C 2346 2006-12-03 18:05:25Z max $ */

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
pmshl_xml (str id)
{
  aout << "bool xml_" << id << " (XML_RPC_obj_t *, void *);\n";
}

static void
pmshl (str id)
{
  aout <<
    "void *" << id << "_alloc ();\n"
    XDR_RETURN " xdr_" << id << " (XDR *, void *);\n";
  pmshl_xml (id);
}

static str
decltype (const rpc_decl *d)
{
  if (d->type == "string")
    return strbuf () << "rpc_str<" << d->bound << ">";
  else if (d->type == "opaque")
    switch (d->qual) {
    case rpc_decl::ARRAY:
      return strbuf () << "rpc_opaque<" << d->bound << ">";
      break;
    case rpc_decl::VEC:
      return strbuf () << "rpc_bytes<" << d->bound << ">";
      break;
    default:
      panic ("bad rpc_decl qual for opaque (%d)\n", d->qual);
      break;
    }
  else
    switch (d->qual) {
    case rpc_decl::SCALAR:
      return d->type;
      break;
    case rpc_decl::PTR:
      return strbuf () << "rpc_ptr<" << d->type << ">";
      break;
    case rpc_decl::ARRAY:
      return strbuf () << "array<" << d->type << ", " << d->bound << ">";
      break;
    case rpc_decl::VEC:
      return strbuf () << "rpc_vec<" << d->type << ", " << d->bound << ">";
      break;
    default:
      panic ("bad rpc_decl qual (%d)\n", d->qual);
    }
}

static void
pdecl (str prefix, const rpc_decl *d)
{
  str name = d->id;
  aout << prefix << decltype (d) << " " << name << ";\n";
}

static void
dumpstruct_xml (const rpc_sym *s)
{
  const rpc_struct *rs = s->sstruct.addr ();
  aout << "bool rpc_traverse (" XML_OBJ "*t, " << rs->id << " &obj);\n" ;
}

static void
dumpstruct (const rpc_sym *s)
{
  const rpc_struct *rs = s->sstruct.addr ();
  aout << "\nstruct " << rs->id << " {\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++)
    pdecl ("  ", rd);
  aout << "};\n";
  pmshl (rs->id);
  aout << "RPC_STRUCT_DECL (" << rs->id << ")\n";
  // aout << "RPC_TYPE_DECL (" << rs->id << ")\n";

  aout << "\ntemplate<class T> "
       << (rs->decls.size () > 1 ? "" : "inline ") << "bool\n"
       << "rpc_traverse (T &t, " << rs->id << " &obj)\n"
       << "{\n";
  const rpc_decl *rd = rs->decls.base ();
  if (rd < rs->decls.lim ()) {
    aout << "  return rpc_traverse (t, obj." << (rd++)->id << ")";
    while (rd < rs->decls.lim ())
      aout << "\n    && rpc_traverse (t, obj." << (rd++)->id << ")";
    aout << ";\n";
  }
  else
    aout << "  return true;\n";
  aout << "}\n\n";

  dumpstruct_xml (s);
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

#if 0
static void
puniontraverse (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
#if 0
  aout << prefix << "if (obj." << rs->tagid << " != tag)\n";
  if (rt->tag.type == "void")
    aout << prefix << "  obj._base.destroy ();\n";
  else
    aout << prefix << "  obj." << rt->tag.id << ".select ();\n";
#endif
  if (rt->tag.type == "void")
    aout << prefix << "return true;\n";
  else
    aout << prefix << "return rpc_traverse (t, *obj." << rt->tag.id << ");\n";
}

static void
pselect (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type == "void")
    aout << prefix << "_base.destroy ();\n";
  else
    aout << prefix << rt->tag.id << ".select ();\n";
  aout << prefix << "break;\n";
}
#endif

static void
punionmacro (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type == "void")
    aout << prefix << "voidaction; \\\n";
  else
    aout << prefix << "action (" << rt->tag.type << ", "
	 << rt->tag.id << "); \\\n";
  aout << prefix << "break; \\\n";
}

static void
punionmacrodefault (str prefix, const rpc_union *rs)
{
  aout << prefix << "defaction; \\\n";
  aout << prefix << "break; \\\n";
}

static void
dumpunion_xml (const rpc_sym *s)
{
  const rpc_union *rs = s->sunion.addr ();
  aout << "bool rpc_traverse (" XML_OBJ " *t, " << rs->id << " &obj);\n";
}

static void
dumpunion (const rpc_sym *s)
{
  bool hasdefault = false;

  const rpc_union *rs = s->sunion.addr ();
  aout << "\nstruct " << rs->id << " {\n"
       << "  const " << rs->tagtype << " " << rs->tagid << ";\n"
       << "  union {\n"
       << "    union_entry_base _base;\n";
  for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
    if (!rt->swval)
      hasdefault = true;
    if (rt->tagvalid && rt->tag.type != "void") {
      str type = decltype (&rt->tag);
      if (type[type.len ()-1] == '>')
	type = type << " ";
      aout << "    union_entry<" << type << "> "
	   << rt->tag.id << ";\n";
    }
  }
  aout << "  };\n\n";

  aout << "#define rpcunion_tag_" << rs->id << " " << rs->tagid << "\n";
  aout << "#define rpcunion_switch_" << rs->id
       << "(swarg, action, voidaction, defaction) \\\n";
  pswitch ("  ", rs, "swarg", punionmacro, " \\\n", punionmacrodefault);

  aout << "\n"
       << "  " << rs->id << " (" << rs->tagtype << " _tag = ("
       << rs->tagtype << ") 0) : " << rs->tagid << " (_tag)\n"
       << "    { _base.init (); set_" << rs->tagid << " (_tag); }\n"

       << "  " << rs->id << " (" << "const " << rs->id << " &_s)\n"
       << "    : " << rs->tagid << " (_s." << rs->tagid << ")\n"
       << "    { _base.init (_s._base); }\n"
       << "  ~" << rs->id << " () { _base.destroy (); }\n"
       << "  " << rs->id << " &operator= (const " << rs->id << " &_s) {\n"
       << "    const_cast<" << rs->tagtype << " &> ("
       << rs->tagid << ") = _s." << rs->tagid << ";\n"
       << "    _base.assign (_s._base);\n"
       << "    return *this;\n"
       << "  }\n\n";

  aout << "  void set_" << rs->tagid << " (" << rs->tagtype << " _tag) {\n"
       << "    const_cast<" << rs->tagtype << " &> (" << rs->tagid
       << ") = _tag;\n"
       << "    rpcunion_switch_" << rs->id << "\n"
       << "      (_tag, RPCUNION_SET, _base.destroy (), _base.destroy ());\n"
       << "  }\n";

#if 0
  aout << "  void Xstompcast () {\n"
       << "    rpcunion_switch_" << rs->id << "\n"
       << "      (" << rs->tagid << ", RPCUNION_STOMPCAST,\n"
       << "       _base.destroy (), _base.destroy ());\n"
       << "  }\n";
#endif
  aout << "};\n";

  aout << "\ntemplate<class T> bool\n"
       << "rpc_traverse (T &t, " << rs->id << " &obj)\n"
       << "{\n"
       << "  " << rs->tagtype << " tag = obj." << rs->tagid << ";\n"
       << "  if (!rpc_traverse (t, tag))\n"
       << "    return false;\n"
       << "  if (tag != obj." << rs->tagid << ")\n"
       << "    obj.set_" << rs->tagid << " (tag);\n\n"
       << "  rpcunion_switch_" << rs->id << "\n"
       << "    (obj." << rs->tagid << ", RPCUNION_TRAVERSE, "
       << "return true, return false);\n"
       << "  /* gcc 4.0.3 makes buggy warnings without the following line */\n"
       << "  return false;\n"
       << "}\n"
       << "inline bool\n"
       << "rpc_traverse (const stompcast_t &s, " << rs->id << " &obj)\n"
       << "{\n"
       << "  rpcunion_switch_" << rs->id << "\n"
       << "    (obj." << rs->tagid << ", RPCUNION_REC_STOMPCAST,\n"
       << "     obj._base.destroy (); return true, "
       << "obj._base.destroy (); return true;);\n"
       << "  /* gcc 4.0.3 makes buggy warnings without the following line */\n"
       << "  return false;\n"
       << "}\n";
  pmshl (rs->id);
  // aout << "RPC_TYPE_DECL (" << rs->id << ")\n";
  aout << "RPC_UNION_DECL (" << rs->id << ")\n";

  aout << "\n";

  dumpunion_xml (s);
}

static void
dumpenum_xml (const rpc_sym *s)
{
  const rpc_enum *rs = s->senum.addr ();
  aout << "bool rpc_traverse (" XML_OBJ " *t, " << rs->id << " &obj);\n";
}

static void
dumpenum (const rpc_sym *s)
{
  int ctr = 0;
  str lastval;
  const rpc_enum *rs = s->senum.addr ();

  aout << "enum " << rs->id << " {\n";
  for (const rpc_const *rc = rs->tags.base (); rc < rs->tags.lim (); rc++) {
    if (rc->val) {
      lastval = rc->val;
      ctr = 1;
      aout << "  " << rc->id << " = " << rc->val << ",\n";
    }
    else if (lastval && (isdigit (lastval[0]) || lastval[0] == '-'
			 || lastval[0] == '+'))
      aout << "  " << rc->id << " = "
	   << strtol (lastval, NULL, 0) + ctr++ << ",\n";
    else if (lastval)
      aout << "  " << rc->id << " = " << lastval << " + " << ctr++ << ",\n";
    else
      aout << "  " << rc->id << " = " << ctr++ << ",\n";
  }
  aout << "};\n";
  pmshl (rs->id);
  aout << "RPC_ENUM_DECL (" << rs->id << ")\n";

  aout << "\ntemplate<class T> inline bool\n"
       << "rpc_traverse (T &t, " << rs->id << " &obj)\n"
       << "{\n"
       << "  u_int32_t val = obj;\n"
       << "  if (!rpc_traverse (t, val))\n"
       << "    return false;\n"
       << "  obj = " << rs->id << " (val);\n"
       << "  return true;\n"
       << "}\n";

  dumpenum_xml (s);
}

static void
dumptypedef (const rpc_sym *s)
{
  const rpc_decl *rd = s->stypedef.addr ();
  pdecl ("typedef ", rd);
  pmshl (rd->id);
  aout << "RPC_TYPEDEF_DECL (" << rd->id << ")\n";
}

static void
dumpprog_xml (const rpc_program *rs)
{
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    aout << "extern const xml_rpc_program xml_" << rpcprog (rs, rv) << ";\n";
  }
  aout << "\n";
}

static void
dumpprog (const rpc_program *rs)
{
  // aout << "\nenum { " << rs->id << " = " << rs->val << " };\n";
  aout << "#ifndef " << rs->id << "\n"
       << "#define " << rs->id << " " << rs->val << "\n"
       << "#endif /* !" << rs->id << " */\n";
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    aout << "extern const rpc_program " << rpcprog (rs, rv) << ";\n";
    aout << "enum { " << rv->id << " = " << rv->val << " };\n";
    aout << "enum {\n";
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++)
      aout << "  " << rp->id << " = " << rp->val << ",\n";
    aout << "};\n";
    aout << "#define " << rs->id << "_" << rv->val
	 << "_APPLY_NOVOID(macro, void)";
    u_int n = 0;
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      while (n++ < rp->val)
	aout << " \\\n  macro (" << n-1 << ", false, false)";
      aout << " \\\n  macro (" << rp->id << ", " << rp->arg
	   << ", " << rp->res << ")";
    }
    aout << "\n";
    aout << "#define " << rs->id << "_" << rv->val << "_APPLY(macro) \\\n  "
	 << rs->id << "_" << rv->val << "_APPLY_NOVOID(macro, void)\n";
  }
  aout << "\n";
  dumpprog_xml (rs);
}

static str
tolower (const str &in)
{
  strbuf r;
  for (const char *c = in.cstr (); *c; c++) {
    r << char (tolower (*c));
  }
  return r;
}

static void
dump_tmpl_proc_1 (const str &arg, const str &res, const str &fn,
		  const str &spc, bool argpointer, const str &rpc)
{

  const char *dec1 = argpointer ? "*" : "&";
  const char *dec2 = argpointer ? ""  : "&";
  
    
  aout << spc << "template<class C, class E> void\n"
       << spc << fn << "(C c, ";
  if (arg) 
    aout << "const " << arg << " " << dec1 << "arg, ";
  if (res)
    aout << res << " *res, ";
  aout << "E cb)\n";

  aout << spc << "{ c->call (" << rpc << ", ";

  if (arg) aout << dec2 << "arg";
  else     aout << "NULL";
  aout << ", ";

  if (res) aout << "res";
  else     aout << "NULL";

  aout << ", cb); }\n";
}

static void
dump_tmpl_proc (const rpc_proc *rc)
{
  str arg, res;
  str fn = tolower (rc->id);
  if (rc->arg != "void") arg = rc->arg;
  if (rc->res != "void") res = rc->res;
  str spc = "    ";

  dump_tmpl_proc_1 (arg, res, fn, spc, true, rc->id);
  if (arg) 
    dump_tmpl_proc_1 (arg, res, fn, spc, false, rc->id);
  aout << "\n";

}

static void
dumpnamespace (const rpc_sym *s)
{
  const rpc_namespace *ns = s->snamespace.addr ();
  for (size_t i = 0; i < ns->progs.size (); i++) {
    dumpprog (&ns->progs[i]);
  }
  aout << "namespace " << ns->id << " {\n";
  for (const rpc_program *rp = ns->progs.base (); 
       rp < ns->progs.lim (); rp++) {
    for (const rpc_vers *rv = rp->vers.base ();
	 rv < rp->vers.lim (); rv++ ) {
      str n = rpcprog (rp, rv);
      aout << "  namespace " << n << " {\n";
      for (const rpc_proc *rc = rv->procs.base ();
	   rc < rv->procs.lim (); rc++) {

	dump_tmpl_proc (rc);

      }
      aout << "  };\n"; // rpcprog (p, v);
    }
  }
  aout << "};\n"; // ns->id
}

static void
dumpsym (const rpc_sym *s)
{
  switch (s->type) {
  case rpc_sym::CONST:
    aout << "enum { " << s->sconst->id
	 << " = " << s->sconst->val << " };\n";
    break;
  case rpc_sym::STRUCT:
    dumpstruct (s);
    break;
  case rpc_sym::UNION:
    dumpunion (s);
    break;
  case rpc_sym::ENUM:
    dumpenum (s);
    break;
  case rpc_sym::TYPEDEF:
    dumptypedef (s);
    break;
  case rpc_sym::PROGRAM:
    dumpprog (s->sprogram);
    break;
  case rpc_sym::NAMESPACE:
    dumpnamespace (s);
    break;
  case rpc_sym::LITERAL:
    aout << *s->sliteral << "\n";
    break;
  default:
    break;
  }
}



static str
makeguard (str fname)
{
  strbuf guard;
  const char *p;

  if ((p = strrchr (fname, '/')))
    p++;
  else p = fname;

  guard << "__RPCC_";
  while (char c = *p++) {
    if (isalnum (c))
      c = toupper (c);
    else
      c = '_';
    guard << c;
  }
  guard << "_INCLUDED__";

  return guard;
}

void
genheader (str fname)
{
  str guard = makeguard (fname);

  aout << "// -*-c++-*-\n"
       << "/* This file was automatically generated by rpcc. */\n\n"
       << "#ifndef " << guard << "\n"
       << "#define " << guard << " 1\n\n"
       << "#include \"xdrmisc.h\"\n"
       << "#include \"okxmlxlate.h\"\n";

  int last = rpc_sym::LITERAL;
  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++) {
    if (last != s->type
	|| last == rpc_sym::PROGRAM
	|| last == rpc_sym::TYPEDEF
	|| last == rpc_sym::STRUCT
	|| last == rpc_sym::UNION
	|| last == rpc_sym::ENUM)
      aout << "\n";
    last = s->type;
    dumpsym (s);
  }

  aout << "extern xml_rpc_file " << stripfname (fname, false)
       << "_rpc_file;\n\n";

  aout << "#endif /* !" << guard << " */\n";
}
