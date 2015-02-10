#include "abson.h"
#include "pub3expr.h"
#include <limits>

namespace okmongo {
    template <size_t Sz>
    struct KeyHelper<rpc_bytes<Sz>> {
        static int32_t len(const rpc_bytes<Sz> &s) {
            return static_cast<int32_t>(s.size());
        }
        static const char *data(const rpc_bytes<Sz> &s) { return s.base(); }
    };
}  // namespace okmongo

// Those macros are only used to hint the compiler's optimiser
// It is safe to define those as:
// #define LIKELY(x) x
// #define UNLIKELY(x) x

#define LIKELY(x) __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0)

namespace {
// Adapted from markus kuhn (http://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c)
// if modified is true then we are checking that the input string is valid.
// `modified utf8` string. We are a bit overly strict since we don't accept
// surrogates

// If we ever find ourselves yearning for more speed we can use:
// https://floodyberry.wordpress.com/2007/04/14/utf-8-conversion-tricks
// For long strings we could also load (aligned) chunks in int64 and use a
// bitmask to check that all the bytes are ascii
// We pass `modified` as a template argument to make sure that the function
// gets specialised properly for the `false` case.
template<bool modified=false>
bool utf8_check_core(const char *in, size_t len) {
    assert(in != nullptr || len == 0);
    const unsigned char *s = reinterpret_cast<const unsigned char *>(in);
    const unsigned char *end = s + len;
    while (s < end) {
        if (UNLIKELY(*s == 0x00 && modified)) {
            return false;
        } else if (LIKELY(*s < 0x80)) { /* 0xxxxxxx */
            s++;
        } else if ((s[0] & 0xe0) == 0xc0) {

            /* 110XXXXx 10xxxxxx */
            if (UNLIKELY(s + 2 > end)) {
                return false;
            }
            if (UNLIKELY((s[1] & 0xc0) != 0x80 ||
                         (s[0] & 0xfe) == 0xc0)) { /* overlong */
                if (!(modified && s[0] == 0xc0 && s[1] == 0x80)) {
                    return false;
                }
            }
            s += 2;
        } else if ((s[0] & 0xf0) == 0xe0) {

            /* 1110XXXX 10Xxxxxx 10xxxxxx */
            if ((s + 3 > end) || (s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) || /* overlong? */
                (s[0] == 0xed && (s[1] & 0xe0) == 0xa0) || /* surrogate? */
                (s[0] == 0xef && s[1] == 0xbf &&
                 (s[2] & 0xfe) == 0xbe)) { /* U+FFFE or U+FFFF? */
                return false;
            }
            s += 3;
        } else if ((s[0] & 0xf8) == 0xf0) {

            /* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
            if ((s + 4) > end || (s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80 ||
                (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) || /* overlong? */
                (s[0] == 0xf4 && s[1] > 0x8f) ||
                s[0] > 0xf4) { /* > U+10FFFF? */
                return false;
            }
            s += 4;
        } else {
            return false;
        }
    }
    return true;
}
} // namespace


bool utf8_check(const char *in, size_t len) {
    return utf8_check_core<false>(in, len);
}

bool modified_utf8_check(const char *in, size_t len) {
    return utf8_check_core<true>(in, len);
}

namespace {

bool dump_dict_cnt(okmongo::BsonWriter *w, xpub3_json_dict_t &dict);
bool dump_list_cnt(okmongo::BsonWriter *w, xpub3_json_list_t &dict);

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
    // TODO: check that the key is valid modified utf-8
    switch (json.typ) {
        case XPUB3_JSON_BOOL:
            w->Element(k, *json.json_bool);
            return true;
        case XPUB3_JSON_INT32:
            w->Element(k, *json.json_int32);
            return true;
        case XPUB3_JSON_UINT32:
            write_uint32(w, k, *json.json_uint32);
            return true;
        case XPUB3_JSON_UINT64:
            write_uint64(w, k, *json.json_uint64);
            return true;
        case XPUB3_JSON_INT64:
            w->Element(k, *json.json_int64);
            return true;
        case XPUB3_JSON_DOUBLE:
            w->Element(k, strtod(json.json_double->val.cstr(), nullptr));
            return true;
        case XPUB3_JSON_NULL:
            w->Element(k, nullptr);
            return true;
        case XPUB3_JSON_DICT:
            w->PushDocument(k);
            if (!dump_dict_cnt(w, *json.json_dict)) {
                return false;
            }
            w->Pop();
            return true;
        case XPUB3_JSON_LIST:
            w->PushArray(k);
            if (!dump_list_cnt(w, *json.json_list)) {
                return false;
            }
            w->Pop();
            return true;
        // TODO: check that the data is valid utf8
        case XPUB3_JSON_STRING:
            if (!utf8_check(json.json_string->base(),
                            json.json_string->size())) {
                warn << __func__ << "json string wasn't valid utf8\n";
                return false;
            }
            w->Element(k, json.json_string->base(), json.json_string->size());
            return true;
        case XPUB3_JSON_ERROR:
            return false;
    }
}

bool dump_dict_cnt(okmongo::BsonWriter *w, xpub3_json_dict_t &dict) {
    for (auto &x : dict.entries) {
        if (!modified_utf8_check(x.key.base(), x.key.size())) {
            warn << __func__ << " json_dict key wasn't valid modified utf8\n";
            return false;
        }
        if (!dump_key_value(w, x.key, *x.value)) {
            return false;
        }
    }
    return true;
}

bool dump_list_cnt(okmongo::BsonWriter *w, xpub3_json_list_t &lst) {
    int32_t i = 0;
    for (auto &x : lst.entries) {
        if (!dump_key_value(w, i++, x)) {
            return false;
        }
    }
    return true;
}
} // namespace


bool pub_fields(okmongo::BsonWriter *w, const pub3::expr_dict_t &pub) {
    xpub3_json_t json;
    pub.to_xdr(&json);
    if (json.typ != XPUB3_JSON_DICT) {
        // Not a dict
        return false;
    }
    return dump_dict_cnt(w, *json.json_dict);
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
    bool ok = dump_dict_cnt(w, *json.json_dict);
    w->Pop();
    return ok;
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

void rpc_bson_writer::error(const char *fld, str s) {
    if (error_) {
        return;
    }
    strbuf buff ("[");
    bool empty = true;
    for (const auto& e: stack) {
        if (!empty) {
            buff << "::";
        }
        empty = false;
        buff << e.key;
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
        case okmongo::BsonTag::kBindata:
            return "Bindata";
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
    assert(tg != v.Tag());
    strbuf res("expected %s got %s",
               a_or_an(to_str(tg)).cstr(),
               a_or_an(to_str(v.Tag())).cstr());
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

const okmongo::BsonValue rpc_bson_reader::get(const char *k) {
    assert(!empty());
    const auto &b = stack_.back();
    auto res = (k)? b.val.GetField(k) : b.val;
    if (res.Empty()) {
        error(k, "missing field");
    }
    return res;
}

const okmongo::BsonValue rpc_bson_reader::get(const char *k,
                                              okmongo::BsonTag tag) {
    auto res = get(k);
    if (res.Empty()) {
        return res;
    }
    if (res.Tag() != tag) {
        error(k, tag);
    }
    return res;
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
    auto v = r.get(k, okmongo::BsonTag::kInt64);
    if (v.Empty()) {
        return false;
    }
    int64_t i = v.GetInt64();
    if (i < 0 ||
        i > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
        r.error(k, "uint32 out of bounds");
        return false;
    }
    u = static_cast<uint32_t>(i);
    return true;
}

bool rpc_traverse(rpc_bson_reader &r, uint64_t &u, const char *k) {
    auto v = r.get(k, okmongo::BsonTag::kInt64);
    if (v.Empty()) {
        return false;
    }
    int64_t i = v.GetInt64();
    u = remove_sign(i);
    return true;
}

#define SIMPLE_TRAV(T, BSON_NAME)                                \
    bool rpc_traverse(rpc_bson_reader &r, T &t, const char *k) { \
        auto v = r.get(k, okmongo::BsonTag::k##BSON_NAME);       \
        if (v.Empty()) {                                         \
            return false;                                        \
        }                                                        \
        t = v.Get##BSON_NAME();                                  \
        return true;                                             \
    }

SIMPLE_TRAV(bool, Bool)
SIMPLE_TRAV(double, Double)
SIMPLE_TRAV(int64_t, Int64)
SIMPLE_TRAV(int32_t, Int32)

#undef SIMPLE_TRAV
