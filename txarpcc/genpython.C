/* $Id$ */

 /*
  *
  * Copyright (C) 2003 Sameer Ajmani (ajmani@csail.mit.edu)
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

#include <assert.h>
#include "rpcc.h"

static str
mangle(const str id)
{
   if (id == "and"
       || id == "del"
       || id == "for"
       || id == "is"
       || id == "raise"
       || id == "assert"
       || id == "elif"
       || id == "from"
       || id == "lambda"
       || id == "return"
       || id == "break"
       || id == "else"
       || id == "global"
       || id == "not"
       || id == "try"
       || id == "class"
       || id == "except"
       || id == "if"
       || id == "or"
       || id == "while"
       || id == "continue"
       || id == "exec"
       || id == "import"
       || id == "pass"
       || id == "yield"
       || id == "def"
       || id == "finally"
       || id == "in"
       || id == "print") {
     return strbuf() << "py_" << id;
   } else {
     return id;
   }
}

static str
packitem(const str type, const str argname)
{
   return strbuf() << "pack_" << type << "(p, " << argname << ")";
}

static str
unpackitem(const str type)
{
   return strbuf() << "unpack_" << type << "(u)";
}

static str
packmethod (const rpc_decl *d, str argname)
{
   if (d->type == "string") {
     return strbuf () << "p.pack_string(" << argname << ")";
   } else if (d->type == "opaque") {
     switch (d->qual) {
     case rpc_decl::ARRAY:
       return strbuf () << "p.pack_fopaque(" << d->bound << ", "
                        << argname << ")";
       break;
     case rpc_decl::VEC:
       return strbuf () << "p.pack_opaque(" << argname << ")";
       break;
     default:
       panic ("bad rpc_decl qual for opaque (%d)\n", d->qual);
       break;
     }
   } else {
     switch (d->qual) {
     case rpc_decl::SCALAR:
       return packitem(d->type, argname);
       break;
     case rpc_decl::PTR:
       return strbuf () << "pack_ptr(p, " << argname
                        << ", lambda x: " << packitem(d->type, "x") << ")";
       break;
     case rpc_decl::ARRAY:
       return strbuf () << "p.pack_farray(" << d->bound << ", " << argname
                        << ", lambda x: " << packitem(d->type, "x") << ")";
       break;
     case rpc_decl::VEC:
       return strbuf () << "p.pack_array(" << argname
                        << ", lambda x: " << packitem(d->type, "x") << ")";
       break;
     default:
       panic ("bad rpc_decl qual (%d)\n", d->qual);
     }
   }
}

static str
unpackmethod (const rpc_decl *d)
{
   if (d->type == "string") {
     return "u.unpack_string()";
   } else if (d->type == "opaque") {
     switch (d->qual) {
     case rpc_decl::ARRAY:
       return strbuf () << "u.unpack_fopaque(" << d->bound << ")";
       break;
     case rpc_decl::VEC:
       return "u.unpack_opaque()";
       break;
     default:
       panic ("bad rpc_decl qual for opaque (%d)\n", d->qual);
       break;
     }
   } else {
     switch (d->qual) {
     case rpc_decl::SCALAR:
       return unpackitem(d->type);
       break;
     case rpc_decl::PTR:
       return strbuf () << "unpack_ptr(u, lambda : "
                        << unpackitem(d->type) << ")";
       break;
     case rpc_decl::ARRAY:
       return strbuf () << "u.unpack_farray(" << d->bound
                        << ", lambda : " << unpackitem(d->type) << ")";
       break;
     case rpc_decl::VEC:
       return strbuf () << "u.unpack_array(lambda : "
                        << unpackitem(d->type) << ")";
       break;
     default:
       panic ("bad rpc_decl qual (%d)\n", d->qual);
     }
   }
}

static void
dumpstruct (const rpc_sym *s)
{ // TODO: create __init__
   // TODO: check that no two slots have the same name
   const rpc_struct *rs = s->sstruct.addr ();
   aout << "class " << mangle(rs->id) << "(object):\n";
   // class slots
   aout << "\t__slots__ = [ ";
   for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
     if (rd > rs->decls.base ()) {
       aout << ", ";
     }
     aout << "'" << mangle(rd->id) << "'";
   }
   aout << " ]\n";
   // check method
   aout << "\tdef check(self):\n"
        << "\t\tpass\n";
   for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
     if (rd->qual != rpc_decl::PTR)
       aout << "\t\tassert self." << mangle(rd->id) << " is not None\n";
   }
   // __eq__ and __ne__ methods
   aout << "\tdef __eq__(self, other):\n";
   for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
     if (rd->qual != rpc_decl::PTR)
       aout << "\t\tif not self." << mangle(rd->id)
            << " == other." << mangle(rd->id) << ": return 0\n";
   }
   aout << "\t\treturn 1\n"
        << "\tdef __ne__(self, other):\n"
        << "\t\treturn not self == other\n";
   // pack method
   aout << "def pack_" << mangle(rs->id) << "(p, o):\n"
        << "\to.check()\n";
   for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
     aout << "\t" << packmethod(rd, strbuf() << "o." << mangle(rd->id))
          << "\n";
   }
   // unpack method
   aout << "def unpack_" << mangle(rs->id) << "(u):\n"
        << "\to = " << mangle(rs->id) << "()\n";
   for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
     aout << "\to." << mangle(rd->id) << " = " << unpackmethod(rd) << "\n";
   }
   aout << "\to.check()\n"
        << "\treturn o\n";
}

static void
dumpunion (const rpc_sym *s)
{ // TODO: create __init__
   // TODO: check that no two slots have the same name
   rpc_utag *thedefault = NULL;
   const rpc_union *rs = s->sunion.addr ();

   aout << "class " << mangle(rs->id) << "(object):\n";
   // class slots
   aout << "\t__slots__ = [ "
        << "'" << rs->tagid << "'";
   for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
     if (!rt->tagvalid)
       continue;
     if (!rt->swval)
       thedefault = const_cast<rpc_utag *>(rt);
     if (rt->tag.type != "void")
       aout << ", '" << rt->tag.id << "'";
   }
   aout << " ]\n";
   // check method
   aout << "\tdef check(self):\n"
        << "\t\tpass\n";
   if (!thedefault)
     aout << "\t\tassert self." << rs->tagid << " is not None\n";
   bool first = true;
   for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
     if (!rt->tagvalid || rt->tag.type == "void" || !rt->swval)
       continue;
     if (first) {
       aout << "\t\tif ";
       first = false;
     } else {
       aout << "\t\telif ";
     }
     aout << "self." << rs->tagid << " == " << rt->swval << ":\n"
          << "\t\t\tassert self." << rt->tag.id << " is not None\n";
   }
   if (thedefault && thedefault->tag.type != "void") {
     if (rs->cases.size() > 1)
       aout << "\telse:\n\t";
     aout << "\t\tassert self." << thedefault->tag.id << " is not None\n";
   }
   // __eq__ and __ne__ methods
   aout << "\tdef __eq__(self, other):\n"
        << "\t\tif not self." << rs->tagid
        << " == other." << rs->tagid << ": return 0\n";
   first = true;
   for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
     if (!rt->tagvalid || rt->tag.type == "void" || !rt->swval)
       continue;
     if (first) {
       aout << "\t\tif ";
       first = false;
     } else {
       aout << "\t\telif ";
     }
     aout << "self." << rs->tagid << " == " << rt->swval << ":\n"
          << "\t\t\tif not self." << rt->tag.id
          << " == other." << rt->tag.id << ": return 0\n";
   }
   if (thedefault && thedefault->tag.type != "void") {
     if (rs->cases.size() > 1)
       aout << "\telse:\n\t";
     aout << "\t\tif not self." << thedefault->tag.id
          << " == other." << thedefault->tag.id << ": return 0\n";
   }
   aout << "\t\treturn 1\n"
        << "\tdef __ne__(self, other):\n"
        << "\t\treturn not self == other\n";
   // pack method
   aout << "def pack_" << mangle(rs->id) << "(p, o):\n"
        << "\to.check()\n"
        << "\t"
        << packitem(rs->tagtype, strbuf() << "o." << rs->tagid)
        << "\n";
   first = true;
   for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
     if (!rt->tagvalid || rt->tag.type == "void" || !rt->swval)
       continue;
     if (first) {
       aout << "\tif ";
       first = false;
     } else {
       aout << "\telif ";
     }
     aout << "o." << rs->tagid << " == " << rt->swval << ":\n"
          << "\t\t"
          << packmethod(&rt->tag, strbuf() << "o." << rt->tag.id)
          << "\n";
   }
   if (thedefault && thedefault->tag.type != "void") {
     if (rs->cases.size() > 1)
       aout << "\telse:\n\t";
     aout << "\t"
          << packmethod(&thedefault->tag,
                        strbuf() << "o." << thedefault->tag.id)
          << "\n";
   }
   // unpack method
   aout << "def unpack_" << mangle(rs->id) << "(u):\n"
        << "\to = " << mangle(rs->id) << "()\n"
        << "\to." << rs->tagid << " = "
        << unpackitem(rs->tagtype) << "\n";
   first = true;
   for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
     if (!rt->tagvalid || rt->tag.type == "void" || !rt->swval)
       continue;
     if (first) {
       aout << "\tif ";
       first = false;
     } else {
       aout << "\telif ";
     }
     aout << "o." << rs->tagid << " == " << rt->swval << ":\n"
          << "\t\to." << rt->tag.id << " = "
          << unpackmethod(&rt->tag) << "\n";
   }
   if (thedefault && thedefault->tag.type != "void") {
     if (rs->cases.size() > 1)
       aout << "\telse:\n\t";
     aout << "\to." << thedefault->tag.id << " = "
          << unpackmethod(&thedefault->tag) << "\n";
   }
   aout << "\to.check()\n"
        << "\treturn o\n";
}

static void
dumpenum (const rpc_sym *s)
{ // TODO: check that no two mangle(rc->id)s have the same name
   // TODO: check that enum only takes allowed values
   int ctr = 0;
   str lastval;
   const rpc_enum *rs = s->senum.addr ();
   aout << "def pack_" << mangle(rs->id) << "(p, o):\n"
        << "\tp.pack_uint(o)\n"
        << "def unpack_" << mangle(rs->id) << "(u):\n"
        << "\treturn u.unpack_uint()\n";
   if (rs->tags.size())
     aout << "\n";
   for (const rpc_const *rc = rs->tags.base (); rc < rs->tags.lim (); rc++) {
     if (rc->val) {
       lastval = rc->val;
       ctr = 1;
       aout << mangle(rc->id) << " = " << rc->val << "\n";
     }
     else if (lastval && (isdigit (lastval[0]) || lastval[0] == '-'
                          || lastval[0] == '+'))
       aout << mangle(rc->id) << " = "
            << strtol (lastval, NULL, 0) + ctr++ << "\n";
     else if (lastval)
       aout << mangle(rc->id) << " = " << lastval << " + " << ctr++ << "\n";
     else
       aout << mangle(rc->id) << " = " << ctr++ << "\n";
   }
}

static void
dumptypedef (const rpc_sym *s)
{
   const rpc_decl *rd = s->stypedef.addr ();
   aout << "def pack_" << mangle(rd->id) << "(p, o):\n"
        << "\t" << packmethod(rd, "o") << "\n"
        << "def unpack_" << mangle(rd->id) << "(u):\n"
        << "\treturn " << unpackmethod(rd) << "\n";
}

static void
dumpprog (const rpc_sym *s)
{
   const rpc_program_p *rs = s->sprogram.addr ();
   aout << mangle(rs->id)  << " = " << rs->val << "\n"
        << "programs[" << mangle(rs->id)  << "] = {}\n";
   for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
     aout << mangle(rv->id) << " = " << rv->val << "\n"
          << "programs[" << mangle(rs->id)  << "]["
          << mangle(rv->id) << "] = {}\n";
       for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim ();
            rp++) {
         aout << mangle(rp->id) << " = " << rp->val << "\n"
              << "programs[" << mangle(rs->id)  << "]["
              << mangle(rv->id) << "]["
              << mangle(rp->id) << "] = proc = Procedure()\n"
              << "proc.pack_arg = pack_" << rp->arg << "\n"
              << "proc.unpack_arg = unpack_" << rp->arg << "\n"
              << "proc.pack_res = pack_" << rp->res << "\n"
              << "proc.unpack_res = unpack_" << rp->res << "\n";
       }
   }
   aout << "\n";
}

static void
dumpsym (const rpc_sym *s)
{
   switch (s->type) {
   case rpc_sym::CONST:
     aout << mangle(s->sconst->id) << " = " << s->sconst->val << "\n";
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
     dumpprog (s);
     break;
   case rpc_sym::LITERAL:
     aout << *s->sliteral << "\n";
     break;
   default:
     break;
   }
}

void
deffns (str type, str packbody, str unpackbody)
{
   aout << "def pack_" << type << "(p, o):\n"
        << "\t" << packbody << "\n"
        << "def unpack_" << type << "(u):\n"
        << "\treturn " << unpackbody << "\n\n";
}

void
genpython (str fname)
{
   aout << "# -*-python-*-\n"
        << "# This file was automatically generated by rpcc.\n\n"
        << "class Procedure(object):\n"
        << "\t__slots__ = [ 'pack_arg', 'unpack_arg',"
        << " 'pack_res', 'unpack_res' ]\n\n"
        << "programs = {}\n\n";

   aout << "def pack_ptr(p, o, packf):\n"
        << "\tif o is None:\n"
        << "\t\tp.pack_uint(0)\n"
        << "\telse:\n"
        << "\t\tp.pack_uint(1)\n"
        << "\t\tpackf(o)\n"
        << "def unpack_ptr(u, unpackf):\n"
        << "\tbit = u.unpack_uint()\n"
        << "\tif bit:\n"
        << "\t\treturn unpackf()\n"
        << "\telse:\n"
        << "\t\treturn None\n\n";

   deffns("void", "pass", "None");

   str types[] = {"int", "uint", "hyper", "uhyper",
                  "float", "double", "bool"};
   for (int i = 0; i < 7; i++) {
     deffns(types[i], strbuf() << "p.pack_" << types[i] << "(o)",
            strbuf() << "u.unpack_" << types[i] << "()");
   }
   deffns("u_int32_t", "p.pack_uint(o)", "u.unpack_uint()");
   deffns("int32_t", "p.pack_int(o)", "u.unpack_int()");
   deffns("u_int64_t", "p.pack_uhyper(o)", "u.unpack_uhyper()");
   deffns("int64_t", "p.pack_hyper(o)", "u.unpack_hyper()");

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
}


