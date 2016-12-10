#include "rpcc.h"
// These forward declarations are missing in sfs...
struct hostent;
#include "pub3obj.h"

template<typename E>
static pub3::obj_list_t to_obj(const vec<E>& v);

static pub3::obj_t to_obj(const xmlrpcc::rpc_decl &d) {
    pub3::obj_dict_t res;
    res("node_type") = "decl";
    res("id") = d.id;
    switch(d.qual) {
    case xmlrpcc::rpc_decl::SCALAR:
        res("qual") = "SCALAR";
        break;
    case xmlrpcc::rpc_decl::PTR:
        res("qual") = "PTR";
        break;
    case xmlrpcc::rpc_decl::ARRAY:
        res("qual") = "ARRAY";
        break;
    case xmlrpcc::rpc_decl::VEC:
        res("qual") = "VEC";
        break;
    }
    res("type") = d.type;
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_const &c) {
    pub3::obj_dict_t res;
    res("node_type") = "const";
    res("id") = c.id;
    res("val") = c.val;
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_struct &s) {
    pub3::obj_dict_t res;
    res("node_type") = "struct";
    res("id") = s.id;
    res("decls") = to_obj(s.decls);
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_enum &e) {
    pub3::obj_dict_t res;
    res("node_type") = "enum";
    res("id") = e.id;
    res("tags") = to_obj(e.tags);
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_utag &u) {
    pub3::obj_dict_t res;
    res("node_type") = "utag";
    res("tag") = to_obj(u.tag);
    res("swval") = u.swval;
    res("tagvalid") = u.tagvalid;
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_union &u) {
    pub3::obj_dict_t res;
    res("node_type") = "union";
    res("id") = u.id;
    res("tagtype") = u.tagtype;
    res("tagid") = u.tagid;
    res("cases") = to_obj(u.cases);
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_arg &a) {
    pub3::obj_dict_t res;
    res("node_type") = "arg";
    res("type") = a.type;
    res("compressed") = a.compressed;
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_proc &p) {
    pub3::obj_dict_t res;
    res("node_type") = "proc";
    res("id") = p.id;
    res("val") = p.val;
    res("arg") = to_obj(p.arg);
    res("res") = to_obj(p.res);
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_vers &v) {
    pub3::obj_dict_t res;
    res("node_type") = "vers";
    res("id") = v.id;
    res("val") = v.val;
    res("procs") = to_obj(v.procs);
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_program &p) {
    pub3::obj_dict_t res;
    res("node_type") = "program";
    res("id") = p.id;
    res("val") = p.val;
    res("vers") = to_obj(p.vers);
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_namespace &n) {
    pub3::obj_dict_t res;
    res("node_type") = "namespace";
    res("id") = n.id;
    res("progs") = to_obj(n.progs);
    return res;
}

static pub3::obj_t to_obj(const xmlrpcc::rpc_sym &s) {
    switch (s.type) {
    case xmlrpcc::rpc_sym::CONST:
        return to_obj(*s.sconst);
    case xmlrpcc::rpc_sym::TYPEDEF:
        return to_obj(*s.stypedef);
    case xmlrpcc::rpc_sym::STRUCT:
        return to_obj(*s.sstruct);
    case xmlrpcc::rpc_sym::ENUM:
        return to_obj(*s.senum);
    case xmlrpcc::rpc_sym::UNION:
        return to_obj(*s.sunion);
    case xmlrpcc::rpc_sym::PROGRAM:
        return to_obj(*s.sprogram);
    case xmlrpcc::rpc_sym::LITERAL:
        {
            pub3::obj_dict_t res;
            res("node_type") = "literal";
            res("val") = *s.sliteral;
            return res;
        }
    case xmlrpcc::rpc_sym::NAMESPACE:
        return to_obj(*s.snamespace);
  }
}

template<typename E>
static pub3::obj_list_t to_obj(const vec<E>& v) {
    pub3::obj_list_t res;
    for (const E& e : v) {
        res.push_back(to_obj(e));
    }
    return res;
}

void genjsonast(str) {
    aout << to_obj(symlist).to_str() << "\n";
}
