// -*- mode:c++ -*-
/***
 * This helper contains helper functions to read in values that are serialised
 * as bson. The helper functions work for pub objects or any types that has
 * rpc_traverse functions defined (e.g.: types defined in prot files).
 */

#pragma once
#include "pub3expr.h"
#include <okmongo/bson.h>


bool pub_to_bson(okmongo::BsonWriter *w, ptr<pub3::expr_t> pub);
bool pub_fields(okmongo::BsonWriter *w, const pub3::expr_dict_t &pub);

ptr<pub3::expr_t> bson_to_pub(const str& s);

namespace okmongo {
    template<>
    struct KeyHelper<str> {
        static int32_t len(const str &s) {
            return static_cast<int32_t>(s.len());
        }
        static const char *data(const str &s) { return s.cstr(); }
    };

    template <size_t Sz>
    struct KeyHelper<rpc_bytes<Sz>> {
        static int32_t len(const rpc_bytes<Sz> &s) {
            return static_cast<int32_t>(s.size());
        }
        static const char *data(const rpc_bytes<Sz> &s) { return s.base(); }
    };

}  // namespace okmongo

// This class factors in the callbacks used to read bson values in. It is used
// both by `bson_expr_reader` and the query reader in ...

template<class Parent>
class bson_expr_reader_engine : public Parent {
protected:
    // We could use an mstr but this is more convenient
    std::string buff_;
    str key;
    struct stack_elt_t {
        ptr<pub3::expr_list_t> lst;
        ptr<pub3::expr_dict_t> dict;
    };
    vec<stack_elt_t> stack_;

    str get_buff_cnt();
    void field(ptr<pub3::expr_t> e);

    ptr<pub3::expr_dict_t> res_;

    str error_;

public:
    void EmitFieldName(const char *data, const size_t len);
    void EmitError(str);
    void EmitOpenDoc();
    void EmitClose();
    void EmitOpenArray();

    void EmitInt32(int32_t i);
    void EmitInt64(int64_t i);
    void EmitBool(bool b);

    void EmitDouble(double d);

    void EmitNull();

    void EmitObjectId(const char *s);

    void EmitUtf8(const char *s, size_t len);

    ptr<pub3::expr_t> get_res() { return res_; }
};

// This is a custom reader that parses pub_exprs in...
class bson_expr_reader : public bson_expr_reader_engine<
                                 okmongo::BsonReader<bson_expr_reader>> {};

template<class Parent>
str bson_expr_reader_engine<Parent>::get_buff_cnt() {
    str res(buff_.data(), buff_.size());
    buff_.clear();
    return res;
}

template<class Parent>
void bson_expr_reader_engine<Parent>::field(ptr<pub3::expr_t> e) {
    auto &v = stack_.back();
    if (v.lst) {
        v.lst->push_back(e);
    } else {
        v.dict->insert(key, e);
    }
}

template <class Parent>
void bson_expr_reader_engine<Parent>::EmitFieldName(const char *data,
                                                    const size_t len) {
    if (len == 0) {
        key = get_buff_cnt();
    } else {
        buff_.append(data, len);
    }
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitError(str s) {
    error_ = s;
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitOpenDoc() {
    auto d = pub3::expr_dict_t::alloc();
    if (res_) {
        field(d);
    } else {
        res_ = d;
    }
    stack_.push_back(stack_elt_t{nullptr, d});
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitClose() { stack_.pop_back(); }

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitOpenArray() {
    auto l = pub3::expr_list_t::alloc();
    field(l);
    stack_.push_back(stack_elt_t{l, nullptr});
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitInt32(int32_t i) {
    field(pub3::expr_int_t::alloc(i));
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitInt64(int64_t i) {
    field(pub3::expr_int_t::alloc(i));
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitBool(bool b) {
    field(pub3::expr_bool_t::alloc(b));
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitDouble(double d) {
    field(pub3::expr_double_t::alloc(d));
}

template <class Parent>
void bson_expr_reader_engine<Parent>::EmitNull() {
    field(pub3::expr_null_t::alloc());
}

template<class Parent>
void bson_expr_reader_engine<Parent>::EmitObjectId(const char *s) {
    field(pub3::expr_str_t::alloc(str(s, 12)));
}

template <class Parent>
void bson_expr_reader_engine<Parent>::EmitUtf8(const char *s, size_t len) {
    if (len == 0) {
        field(pub3::expr_str_t::alloc(get_buff_cnt()));
    } else {
        buff_.append(s, len);
    }
}

//------------------------------------------------------------------------------

inline uint64_t remove_sign(int64_t i) {
    return static_cast<uint64_t>(i);
}

// casting from unsigned to signed is unspecified (c++ 4.7), this is the inverse
// operation from the cast.
// Adapted from:
//  http://stackoverflow.com/questions/13150449/efficient-unsigned-to-signed-cast-avoiding-implementation-defined-behavior
inline int64_t add_sign(uint64_t u) {
    uint64_t umax_int64 =
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    int64_t min_int64 = std::numeric_limits<int64_t>::min();
    uint64_t umin_int64 = static_cast<uint64_t>(min_int64);

    if (u <= static_cast<uint64_t>(umax_int64))
        return static_cast<int64_t>(u);

    return static_cast<int64_t>(u - umin_int64) + min_int64;
}

template <typename Key>
inline void write_uint32(okmongo::BsonWriter *w, Key k, uint32_t i) {
    return w->Element(k, static_cast<int64_t>(i));
}


template <typename Key>
inline void write_uint64(okmongo::BsonWriter *w, Key k, uint64_t src) {
    return w->Element(k, add_sign(src));
}

//-----------------------------------------------------------------------------

enum class bsw_stack_state : uint8_t {
    UNUSED,
    DOCUMENT,
    USED,
};

struct bsw_stack_frame {
    const char* key;
    bsw_stack_state state;
};

class rpc_bson_writer {
public:
    okmongo::BsonWriter *writer;
    vec<bsw_stack_frame> stack;

    explicit rpc_bson_writer(okmongo::BsonWriter *w): writer(w){};
    void descend();
    void close(const char* k);
    const char* get_key(const char* s);
};

void rpc_enter_field(rpc_bson_writer &w, const char *k);
void rpc_exit_field(rpc_bson_writer &w, const char *k);

template <class R, size_t n>
bool rpc_traverse(rpc_bson_writer &w,
                  rpc_vec<R, n> &obj,
                  const char *field) {
    field = w.get_key(field);
    w.writer->PushArray(field);
    const size_t len = obj.size();
    for (size_t i = 0; i < len; i++) {
        std::string k = std::to_string(i);
        if (!rpc_traverse(w, obj[i], k.c_str())) {
            w.writer->Pop();
            return false;
        };
    }
    w.writer->Pop();
    return true;
}

template <class R>
inline bool rpc_traverse(rpc_bson_writer &w, rpc_ptr<R> &obj,
                         const char *field) {
    field = w.get_key(field);
    if (!obj) {
        w.writer->Element(field, nullptr);
        return true;
    }
    return rpc_traverse (w, *obj, field);
}

bool rpc_traverse(rpc_bson_writer &w, bool &v, const char *k = nullptr);
bool rpc_traverse(rpc_bson_writer &w, double &v, const char *k = nullptr);
bool rpc_traverse(rpc_bson_writer &w, int64_t &v, const char *k = nullptr);
bool rpc_traverse(rpc_bson_writer &w, int32_t &v, const char *k = nullptr);
bool rpc_traverse(rpc_bson_writer &w, uint64_t &u, const char *k = nullptr);
bool rpc_traverse(rpc_bson_writer &w, uint32_t &u, const char *k = nullptr);

template <size_t n>
bool rpc_traverse(rpc_bson_writer &w, rpc_str<n> &s, const char *k) {
    k = w.get_key(k);
    w.writer->Element(k, s.cstr(), s.len());
    return true;
}

template <size_t n>
bool rpc_traverse(rpc_bson_writer &w, rpc_bytes<n> &b, const char *k) {
    k = w.get_key(k);
    w.writer->Element(k, b.base(), b.size());
    return true;
}

//------------------------------------------------------------------------------
// XDR reader
//------------------------------------------------------------------------------

// This class is passed to rpc_traverse to read a value from the a raw bson
// value (as a char*).
class rpc_bson_reader {
private:
    str error_;
public:
    struct stack_frame_t {
       const okmongo::BsonValue val;
       const char* field;
    };

    vec<stack_frame_t> stack_;

    void enter_field(const okmongo::BsonValue &v, const char*);
    void exit_field();

    bool empty() const;

    void error(const char * fld, str);
    str get_error() const { return error_; }

    rpc_bson_reader(const okmongo::BsonValue &v);
    okmongo::BsonValue get(const char *k);
    const char* get(const char *k, okmongo::BsonTag t);
};

void rpc_enter_field(rpc_bson_reader &r, const char *k);
void rpc_exit_field(rpc_bson_reader &r, const char *k);

template <class R>
bool rpc_traverse(rpc_bson_reader &r, rpc_ptr<R> &obj, const char *field) {
    auto v = r.get(field);
    if (v.Empty()) {
        r.error(field, "missing field");
        return false;
    }
    if (v.tag() == okmongo::BsonTag::kNull) {
        obj.clear();
        return true;
    }
    r.enter_field(v, field);
    bool res = rpc_traverse (r, *obj.alloc());
    r.exit_field();
    return res;
}

template <class R, size_t n>
bool rpc_traverse(rpc_bson_reader &r, rpc_vec<R, n> &obj,
                  const char *field) {
    auto v = r.get(field);
    bool ok = true;
    if (v.tag() != okmongo::BsonTag::kArray) {
        r.error(field, "not an array");
        return false;
    }
    okmongo::BsonValueIt it(v);
    while (!it.Empty() && ok) {
        r.enter_field(it, field);
        ok = rpc_traverse(r, obj.push_back());
        r.exit_field();
        it.next();
    }
    return ok;
}


template <size_t n>
bool rpc_traverse(rpc_bson_reader &r, rpc_str<n> &s, const char *k) {
    const char* data = r.get(k, okmongo::BsonTag::kUtf8);
    if (!data) {
        r.error(k, "not a string");
        return false;
    }
    int32_t len;
    std::memcpy(&len, data, sizeof(int32_t));
    if (len < 1) {
        r.error(k, "invalid value");
        return false;
    }
    s = rpc_str<n>(data + sizeof(int32_t), len - 1);
    return true;
}

template <size_t n>
bool rpc_traverse(rpc_bson_reader &r, rpc_bytes<n> &s, const char *k) {
    const char* data = r.get(k, okmongo::BsonTag::kUtf8);
    if (!data) {
        r.error(k, "not a string");
        return false;
    }
    int32_t len;
    std::memcpy(&len, data, sizeof(int32_t));
    if (len < 1) {
        r.error(k, "invalid value");
        return false;
    }
    s.setsize(len - 1);
    std::memcpy(s.base(), data + sizeof(int32_t), len - 1);
    //s = rpc_bytes<n>(data + sizeof(int32_t), len - 1);
    return true;
}

bool rpc_traverse(rpc_bson_reader &r, double &v, const char *k = nullptr);
bool rpc_traverse(rpc_bson_reader &r, int32_t &v, const char *k = nullptr);
bool rpc_traverse(rpc_bson_reader &r, int64_t &v, const char *k = nullptr);
bool rpc_traverse(rpc_bson_reader &r, uint32_t &u, const char *k = nullptr);
bool rpc_traverse(rpc_bson_reader &r, uint64_t &v, const char *k = nullptr);

// These are helper macros that allow to override the behaviour of rpc_traverse
// on unions...
#define FLAT_UNION_TRAVERSE_HELPER(_TYPE, _FIELD) rpc_traverse(t, *obj._FIELD)

#define FLAT_UNION_TRAVERSE(_OUT_TYPE, _IN_TYPE, _TAG_FLD)               \
    bool rpc_traverse(_OUT_TYPE &t, _IN_TYPE &obj, const char *field) {  \
        bool ret = true;                                                 \
        rpc_enter_field(t, field);                                       \
        auto tag = obj._TAG_FLD;                                         \
        if (!rpc_traverse(t, tag, #_TAG_FLD)) {                          \
            ret = false;                                                 \
        } else {                                                         \
            if (tag != obj._TAG_FLD) obj.set_##_TAG_FLD(tag);            \
                                                                         \
            rpcunion_switch_##_IN_TYPE(obj._TAG_FLD,                     \
                                       ret = FLAT_UNION_TRAVERSE_HELPER, \
                                       ret = true, ret = false);         \
        }                                                                \
        rpc_exit_field(t, field);                                        \
        return ret;                                                      \
    }
