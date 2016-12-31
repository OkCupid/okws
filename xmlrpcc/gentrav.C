#include "rpcc.h"

str getid(const rpc_sym *s) {
  switch (s->type) {
    case rpc_sym::CONST:
      return s->sconst->id;
    case rpc_sym::STRUCT:
      return s->sstruct->id;
    case rpc_sym::UNION:
      return s->sunion->id;
    case rpc_sym::ENUM:
      return s->senum->id;
    case rpc_sym::TYPEDEF:
      return s->stypedef->id;
    case rpc_sym::PROGRAM:
      return s->sprogram->id;
    case rpc_sym::NAMESPACE:
      return s->snamespace->id;
    case rpc_sym::LITERAL:
      return nullptr;
  }
}

vec<str> template_specialisations{"ptr<v_XDR_t>", "XDR*"};

static str get_trav_name(const rpc_sym *s, bool inl) {
  if (inl) {
    return "rpc_traverse";
  }
  return str(strbuf("rpc_traverse_") << getid(s));
}

static str dump_rpc_trav_struct(const rpc_sym *s, bool inl) {
  const rpc_struct *rs = s->sstruct.addr();
  const char *field = inl ? rpc_field : "const char* field";
  const str fn_name = get_trav_name(s, inl);

  aout << "template<class T> " << (rs->decls.size() > 1 ? "" : "inline ")
       << "bool\n" << fn_name << " (T &t, " << rs->id << " &obj, " << field
       << ")\n"
       << "{\n"
       << "  bool ret = true;\n"
       << "  rpc_enter_field (t, field);\n";

  const rpc_decl *rd = rs->decls.base();
  if (rd < rs->decls.lim()) {
    aout << "  ret = rpc_traverse (t, obj." << rd->id << ", \"" << rd->id
         << "\")";
    rd++;
    for (; rd < rs->decls.lim(); rd++) {
      aout << "\n    && rpc_traverse (t, obj." << rd->id << ", \"" << rd->id
           << "\")";
    }
    aout << ";\n";
  }
  aout << "  rpc_exit_field (t, field);\n"
       << "  return ret;\n"
       << "}\n\n";
  return fn_name;
}

static void punionmacro(str prefix, const rpc_union *rs, const rpc_utag *rt) {
  if (rt->tag.type == "void")
    aout << prefix << "voidaction; \\\n";
  else
    aout << prefix << "action (" << rt->tag.type << ", " << rt->tag.id
         << "); \\\n";
  aout << prefix << "break; \\\n";
}

static void punionmacrodefault(str prefix, const rpc_union *rs) {
  aout << prefix << "defaction; \\\n";
  aout << prefix << "break; \\\n";
}

static str dump_rpc_trav_union(const rpc_sym *s, bool inl) {
  const rpc_union *rs = s->sunion.addr();
  const char *field = inl ? rpc_field : "const char* field";
  const str fn_name = get_trav_name(s, inl);
  const str inl_inst = inl ? "inline " : "";
  aout << "#define rpcunion_tag_" << rs->id << " " << rs->tagid << "\n";
  aout << "#define rpcunion_switch_" << rs->id
       << "(swarg, action, voidaction, defaction) \\\n";
  pswitch("  ", rs, "swarg", punionmacro, " \\\n", punionmacrodefault);

  aout << "\n";

  aout << inl_inst << "void " << rs->id << "::set_" << rs->tagid
       << " (" << rs->tagtype << " _tag) {\n"
       << "    const_cast<" << rs->tagtype << " &> (" << rs->tagid
       << ") = _tag;\n"
       << "    rpcunion_switch_" << rs->id << "\n"
       << "      (_tag, RPCUNION_SET, _base.destroy (), _base.destroy ());\n"
       << "  }\n";

  aout << "template<class T> bool\n" << fn_name << " (T &t, " << rs->id
       << " &obj, " << field << ")\n"
       << "{\n"
       << "  bool ret = true;\n"
       << "  rpc_enter_field (t, field);\n"
       << "  " << rs->tagtype << " tag = obj." << rs->tagid << ";\n"
       << "  if (!rpc_traverse (t, tag, \"" << rs->tagid << "\")) { \n"
       << "    ret = false;\n"
       << "  } else {\n"
       << "    if (tag != obj." << rs->tagid << ")\n"
       << "      obj.set_" << rs->tagid << " (tag);\n\n"
       << "    rpcunion_switch_" << rs->id << "\n"
       << "      (obj." << rs->tagid << ", ret = RPCUNION_TRAVERSE_2, "
       << "ret = true, ret = false);\n"
       << "    /* gcc 4.0.3 makes buggy warnings without the following.. */\n"
       << "  }\n"
       << "  rpc_exit_field (t, field);\n"
       << "  return ret;\n"
       << "}\n\n";

  aout << "#undef rpcunion_tag_" << rs->id << "\n";
  aout << "#undef rpcunion_switch_" << rs->id << "\n\n";

  return fn_name;
}

static str dump_rpc_trav_enum(const rpc_sym *s, bool inl) {
  const rpc_enum *rs = s->senum.addr();
  const char *field = inl ? rpc_field : "const char* field";
  const str fn_name = get_trav_name(s, inl);
  aout << "\ntemplate<class T> inline bool\n" << fn_name << " (T &t, " << rs->id
       << " &obj, " << field << ")\n"
       << "{\n"
       << "  u_int32_t val = obj;\n"
       << "  bool ret = true;\n"
       << "  rpc_enter_field (t, field);\n"
       << "  if (!rpc_traverse (t, val)) {\n"
       << "    ret = false;\n"
       << "  } else {\n"
       << "    obj = " << rs->id << " (val);\n"
       << "  }\n"
       << "  rpc_exit_field (t, field);\n"
       << "  return ret;\n"
       << "}\n\n";
  return fn_name;
}

str gen_rpc_trav(const rpc_sym *s, bool inl) {
  switch (s->type) {
    case rpc_sym::STRUCT:
      return dump_rpc_trav_struct(s, inl);
    case rpc_sym::UNION:
      return dump_rpc_trav_union(s, inl);
    case rpc_sym::ENUM:
      return dump_rpc_trav_enum(s, inl);
    case rpc_sym::TYPEDEF:
    case rpc_sym::PROGRAM:
    case rpc_sym::NAMESPACE:
    case rpc_sym::LITERAL:
    case rpc_sym::CONST:
      return nullptr;
  }
}
