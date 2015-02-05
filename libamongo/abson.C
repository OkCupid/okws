#include "abson.h"
#include "pub3expr.h"
#include <limits>

namespace {

void dump_dict_cnt(okmongo::BsonWriter *w, xpub3_json_dict_t &dict);
void dump_list_cnt(okmongo::BsonWriter *w, xpub3_json_list_t &dict);

template <typename Key>
inline void write_uint32(okmongo::BsonWriter *w, Key k, uint32_t i) {
    return w->Element(k, static_cast<int64_t>(i));
}


template <typename Key>
inline void write_uint64(okmongo::BsonWriter *w, Key k, uint64_t src) {
    return w->Element(k, add_sign(src));
}

template <typename K>
bool dump_key_value(okmongo::BsonWriter *w, K k, xpub3_json_t &json) {
    switch (json.typ) {
        case XPUB3_JSON_BOOL:
            return w->Element(k, *json.json_bool);
        case XPUB3_JSON_INT32:
            return w->Element(k, *json.json_int32);
        case XPUB3_JSON_UINT32:
            return write_uint32(w, k, *json.json_uint32);
        case XPUB3_JSON_UINT64:
            return write_uint64(w, k, *json.json_uint64);
        case XPUB3_JSON_INT64:
            return w->Element(k, *json.json_int64);
        case XPUB3_JSON_DOUBLE:
            return w->Element(k, strtod(json.json_double->val.cstr(), nullptr));
        case XPUB3_JSON_NULL:
            return w->Element(k, nullptr);
        case XPUB3_JSON_DICT:
            w->PushDocument(k);
            dump_dict_cnt(w, *json.json_dict);
            w->Pop();
            return;
        case XPUB3_JSON_LIST:
            w->PushArray(k);
            dump_list_cnt(w, *json.json_list);
            w->Pop();
            return;
        case XPUB3_JSON_STRING:
            return w->Element(k, json.json_string->base(),
                              json.json_string->size());
        case XPUB3_JSON_ERROR:
            return;
    }
}

bool dump_dict_cnt(okmongo::BsonWriter *w, xpub3_json_dict_t &dict) {
    for (auto &x : dict.entries) {
        const char *key = x.key.base();
        dump_key_value(w, key, *x.value);
    }
}

bool dump_list_cnt(okmongo::BsonWriter *w, xpub3_json_list_t &lst) {
    int32_t i = 0;
    for (auto &x : lst.entries) {
        dump_key_value(w, i++, x);
    }
}
} // namespace


bool pub_fields(okmongo::BsonWriter *w, const pub3::expr_dict_t &pub) {
    xpub3_json_t json;
    pub.to_xdr(&json);
    if (json.typ != XPUB3_JSON_DICT) {
        // Not a dict
        return false;
    }
    dump_dict_cnt(w, *json.json_dict);
    return true;
}

bool pub_to_bson(okmongo::BsonWriter *w, ptr<pub3::expr_t> pub) {
    if (!pub) {
        return false;
    }
    xpub3_json_t json;
    pub->to_xdr(&json);
    if (json.typ != XPUB3_JSON_DICT) {
        // Not a dict
        return false;
    }
    w->Document();
    dump_dict_cnt(w, *json.json_dict);
    w->Pop();
    return true;
}

ptr<pub3::expr_t> bson_to_pub(const str &s) {
    bson_expr_reader r;
    const ssize_t consumed = r.Consume(s.cstr(), s.len());
    if (consumed != static_cast<ssize_t>(s.len())) {
        return nullptr;
    }
    return r.get_res();

}

//------------------------------------------------------------------------------

void rpc_bson_writer::descend() {
    if (!stack.empty()) {
        bsw_stack_frame &s = stack.back();
        assert(s.state != bsw_stack_state::USED);
        if (s.state == bsw_stack_state::UNUSED) {
            assert(s.key);
            writer->PushDocument(s.key);
            s.state = bsw_stack_state::DOCUMENT;
        }
    }
}

void rpc_bson_writer::close(const char *k) {
    assert(!stack.empty());
    bsw_stack_frame s = stack.pop_back();
    assert((!k && !s.key) || (strcmp(s.key, k) == 0));
    switch (s.state) {
        case bsw_stack_state::UNUSED:
            assert(s.key);
            writer->PushDocument(s.key);
            writer->Pop();
            break;
        case bsw_stack_state::DOCUMENT:
            writer->Pop();
            break;
        case bsw_stack_state::USED:
            break;
    }
}

const char *rpc_bson_writer::get_key(const char *s) {
    if (s) {
        descend();
        return s;
    }
    assert(!stack.empty());
    bsw_stack_frame &back = stack.back();
    assert(back.key);
    assert(back.state == bsw_stack_state::UNUSED);
    back.state = bsw_stack_state::USED;
    return back.key;
}

void rpc_enter_field(rpc_bson_writer &w, const char *k) {
    if (k!=nullptr) {
        w.descend();
        w.stack.push_back(bsw_stack_frame{k, bsw_stack_state::UNUSED});
    }
}

void rpc_exit_field(rpc_bson_writer &w, const char *k) {
    if (k!=nullptr) {
        w.close(k);
    }
}

#define SIMPLE_TRAV(T)                                           \
    bool rpc_traverse(rpc_bson_writer &w, T &v, const char *k) { \
        k = w.get_key(k);                                        \
        w.writer->Element(k, v);                                 \
        return true;                                             \
    }

SIMPLE_TRAV(bool)
SIMPLE_TRAV(double)
SIMPLE_TRAV(int64_t)
SIMPLE_TRAV(int32_t)

#undef SIMPLE_TRAV

bool rpc_traverse(rpc_bson_writer &w, uint64_t &u, const char *k) {
    k = w.get_key(k);
    write_uint64(w.writer, k, u);
    return true;
}

bool rpc_traverse(rpc_bson_writer &w, uint32_t &u, const char *k) {
    k = w.get_key(k);
    write_uint32(w.writer, k, u);
    return true;
}

//------------------------------------------------------------------------------

rpc_bson_reader::rpc_bson_reader(const okmongo::BsonValue &v) {
    stack_.push_back(stack_frame_t {v, nullptr});
}

void rpc_bson_reader::enter_field(const okmongo::BsonValue &v, const char*fld) {
    stack_.push_back(stack_frame_t {v, fld});
}

void rpc_bson_reader::exit_field() {
    stack_.pop_back();
}

okmongo::BsonValue rpc_bson_reader::get(const char *k) {
    assert(!empty());
    const auto &b = stack_.back();
    if (!k) {
        return b.val;
    }
    return b.val.GetField(k);
}

static const char* to_str(okmongo::BsonTag tg) {
    switch(tg) {
        case okmongo::BsonTag::kDouble:
            return "Double";
        case okmongo::BsonTag::kUtf8:
            return "Utf8";
        case okmongo::BsonTag::kDocument:
            return "Document";
        case okmongo::BsonTag::kArray:
            return "Array";
        case okmongo::BsonTag::kBinData:
            return "BinData";
        case okmongo::BsonTag::kObjectId:
            return "ObjectId";
        case okmongo::BsonTag::kBool:
            return "Bool";
        case okmongo::BsonTag::kUtcDatetime:
            return "UtcDatetime";
        case okmongo::BsonTag::kNull:
            return "Null";
        case okmongo::BsonTag::kRegexp:
            return "Regexp";
        case okmongo::BsonTag::kJs:
            return "Js";
        case okmongo::BsonTag::kScopedJs:
            return "ScopedJs";
        case okmongo::BsonTag::kInt32:
            return "Int32";
        case okmongo::BsonTag::kTimestamp:
            return "Timestamp";
        case okmongo::BsonTag::kInt64:
            return "Int64";
        case okmongo::BsonTag::kMinKey:
            return "MinKey";
        case okmongo::BsonTag::kMaxKey:
            return "MaxKey";
        default:
            return "Unknown";
    }
}

static str a_or_an(const char* in) {
    switch (*in) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
        return str(strbuf("an %s", in));
    default:
        return str(strbuf("a %s", in));
    }
}

void rpc_bson_reader::error(const char *fld, okmongo::BsonTag tg,
                            const char *extra) {
    if (error_) {
        return;
    }
    okmongo::BsonValue v = get(fld);
    if (v.Empty()) {
        return error(fld, "missing field");
    }
    assert(tg != v.tag());
    strbuf res("expected %s got %s",
               a_or_an(to_str(tg)).cstr(),
               a_or_an(to_str(v.tag())).cstr());
    if (extra != nullptr) {
        res << "\nnote: " << extra;
    }
    return error(fld, str(res));
}

void rpc_bson_reader::error(const char *fld, str s) {
    if (error_) {
        return;
    }
    strbuf buff ("[");
    bool empty = true;
    for (const auto& e: stack_) {
        if (e.field) {
            if (!empty) {
                buff << "::";
            }
            empty = false;
            buff << e.field;
        }
    }
    if (fld) {
        if (!empty) {
            buff << "::";
        }
        empty = false;
        buff << fld;
    }
    buff << "] " << s;
    error_ = str(buff);
}

bool rpc_bson_reader::empty() const {
    return stack_.empty();
}

const char* rpc_bson_reader::get(const char *k, okmongo::BsonTag t) {
    assert(!empty());
    const auto fld = get(k);
    if (fld.tag() != t) {
        return nullptr;
    }
    return fld.data();
}

void rpc_enter_field(rpc_bson_reader &r, const char *k) {
    if (k != nullptr) {
        r.enter_field(r.get(k), k);
    }
}

void rpc_exit_field(rpc_bson_reader &r, const char *k) {
    if (k != nullptr) {
        r.exit_field();
    }
}

bool rpc_traverse(rpc_bson_reader &r, uint32_t &u, const char *k) {
    const char *data = r.get(k, okmongo::BsonTag::kInt64);
    if (!data) {
        r.error(k, okmongo::BsonTag::kInt64,
                "(uint32 are represented in bson as int64)");
        return false;
    }
    int64_t i;
    std::memcpy(&i, data, sizeof(int64_t));
    if (i < 0 ||
        i > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
        r.error(k, "uint32 out of bounds");
        return false;
    }
    u = static_cast<uint32_t>(i);
    return true;
}

bool rpc_traverse(rpc_bson_reader &r, uint64_t &v, const char *k) {
    const char *data = r.get(k, okmongo::BsonTag::kInt64);
    if (!data) {
        r.error(k, okmongo::BsonTag::kInt64,
                "uint64 are represented in bson as int64");
        return false;
    }
    int64_t i;
    std::memcpy(&i, data, sizeof(int64_t));
    v = remove_sign(i);
    return true;
}

#define SIMPLE_TRAV(T, TAG)                                      \
    bool rpc_traverse(rpc_bson_reader &r, T &v, const char *k) { \
        const char *data = r.get(k, okmongo::BsonTag::TAG);      \
        if (!data) {                                             \
            r.error(k, okmongo::BsonTag::TAG);                   \
            return false;                                        \
        }                                                        \
        std::memcpy(&v, data, sizeof(T));                        \
        return true;                                             \
    }

SIMPLE_TRAV(bool, kBool)
SIMPLE_TRAV(double, kDouble)
SIMPLE_TRAV(int64_t, kInt64)
SIMPLE_TRAV(int32_t, kInt32)

#undef SIMPLE_TRAV
