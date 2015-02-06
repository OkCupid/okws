// -*- mode:c++ -*-
#include "amisc.h"
#include "pub3.h"
#include "okrfn.h"

#include "abson.h"
#include "amongo.h"

#include "fakeblob_prot.h"

#include <fstream>
#include <limits>

#define WERR  warn << __func__ << "[" << __LINE__ << "] "

static void insert_usr(mongo_connection_t *, str name, uint64_t,
                       pub3::xev_t ev, CLOSURE);

tamed void insert_usr(mongo_connection_t *db, str name, uint64_t id,
                      pub3::xev_t ev) {
    tvars {
        ptr<pub3::expr_dict_t> usr;
        bool ok;
    }
    usr.alloc();
    usr->insert("username", name);
    usr->insert("tab", pub3::expr_uint_t::alloc(id));
    twait {
        db->insert("users", usr, mkevent(ok));
    }
    if (!ok) {
        WERR << "Failed to insert: " << name << "\n";
        exit(1);
    }
    ev->trigger(usr);
}

static void pub_mongo_test(ptr<mongo_connection_t> conn, evv_t, CLOSURE);

tamed void pub_mongo_test(ptr<mongo_connection_t> conn, evv_t ev) {
    tvars {
        ptr<pub3::expr_dict_t> all;
        ptr<pub3::expr_dict_t> usr1_qry;
        ptr<pub3::expr_list_t> res;
        ptr<pub3::expr_t> usr1;
        ptr<pub3::expr_t> usr2;
        bool ok;
    }
    res->alloc();
    // Empty document selects all
    all = pub3::expr_dict_t::alloc();

    // Filling the query for usr1
    usr1_qry = pub3::expr_dict_t::alloc();
    usr1_qry->insert("username", "usr1");

    twait {
        conn->remove("users", all, mkevent(ok));
    }

    if (!ok) {
        WERR << ": failed\n";
        exit(1);
    }

    twait {
        insert_usr(conn, "usr1", 1, mkevent(usr1));
    }
    twait {
        insert_usr(conn, "usr2", 2, mkevent(usr2));
    }

    twait {
        conn->query("users", all, mkevent(res));
    }

    if (res) {
        assert(res->size() == 2);
        str name1 = res->lookup(0)->to_dict()->lookup("username")->to_str();
        str name2 = res->lookup(1)->to_dict()->lookup("username")->to_str();
        assert((name1 == "usr1" && name2 == "usr2") ||
               (name1 == "usr2" && name2 == "usr1"));
    } else {
        WERR << "failed \n";
        exit(1);
    }

    twait {
        conn->query("users", usr1_qry, mkevent(res));
    }

    if (res) {
        assert(res->size() == 1);
        str name = res->lookup(0)->to_dict()->lookup("username")->to_str();
        assert(name == "usr1");
    } else {
        WERR << "failed \n";
        exit(1);
    }

    ev->trigger();
}

//------------------------------------------------------------------------------
// Native test
//------------------------------------------------------------------------------

bool rpc_traverse(rpc_bson_writer &t, blob_t &obj, const char *field = nullptr);
bool rpc_traverse(rpc_bson_reader &t, blob_t &obj, const char *field = nullptr);

FLAT_UNION_TRAVERSE(rpc_bson_writer, blob_t, ver);
FLAT_UNION_TRAVERSE(rpc_bson_reader, blob_t, ver);

// A range (bounds included...)
struct uid_range_t {
    uint64_t low;
    uint64_t high;
};

static void WriteRangeQueryFields(okmongo::BsonWriter *w, const char *fld,
                                  uint64_t low, uint64_t high) {
    assert (low <= high);
    constexpr uint64_t cutoff =
            static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    if (high <= cutoff || (low > cutoff)) {
        w->PushDocument(fld);
        w->Element("$gte", add_sign(low));
        w->Element("$lte", add_sign(high));
        w->Pop();
    } else {
        w->PushArray("$or");
        {
            w->PushDocument(0);
            w->PushDocument(fld);
            w->Element("$gte", add_sign(low));
            w->Pop();
            w->Pop();

            w->PushDocument(1);
            w->PushDocument(fld);
            w->Element("$lte", add_sign(high));
            w->Pop();
            w->Pop();
        }
        w->Pop();
    }
};


namespace okmongo {

    template <>
    bool BsonWriteFields<uid_range_t>(BsonWriter *w,
                                      const uid_range_t &range) {
        WriteRangeQueryFields(w, "userid", range.low, range.high);
        return true;
    }
} // okmongo.

struct query_all_t {};

namespace okmongo {
template <>
bool BsonWriteFields<query_all_t>(BsonWriter *, const query_all_t &) {
    return true;
}
}

void native_mongo_test(mongo_connection_t *, evv_t, CLOSURE);

constexpr uint64_t kInt64Cutoff =
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max());

constexpr uint64_t uidsToInsert = 102400;

tamed void native_mongo_test(mongo_connection_t *conn, evv_t ev) {
    tvars {
        const str col("blobs");
        bool ok;
        vec<blob_t> blobs;
        query_all_t qry;
        uid_range_t range_qry({kInt64Cutoff - 50, kInt64Cutoff + 50});
        uid_wrapper_t sel_uid;
        vec<uid_wrapper_t> sel_res;
        mongo_result_t qres;
        uint64_t cnt(0);
    }
    // Clear it...
    twait {
        conn->remove(col, qry, mkevent(ok));
    }

    if(!ok) {
        WERR << ": delete failed\n";
        exit(1);
    }

    blobs.setsize(5000);

    for (blob_t &blob : blobs) {
        blob.set_ver(BV_V2);
        blob.v2 = {};
    }

    while (cnt < 102400) {
        for (blob_t &blob : blobs) {
            blob.v2->userid = kInt64Cutoff - uidsToInsert / 2 + cnt;
            cnt++;
        }
        twait {
            conn->insert(col, blobs, mkevent(ok));
        }
        if (!ok) {
            WERR << ":Insert failed.\n";
            exit(1);
        }
    }

    // Select only the userids
    sel_uid.userid = 1;

    range_qry.high = kInt64Cutoff - 10;
    twait {
        conn->query<vec<uid_wrapper_t>>(col, range_qry, &sel_res, mkevent(qres),
                                        sel_uid);
    }

    if (!qres.error) {
        vec<uint64_t> res;
        for (const auto &e : sel_res) {
            res.push_back(e.userid);
        }
        std::sort(res.begin(), res.end());
        uint64_t check_cnt = range_qry.low;
        for (uint64_t uid : res) {
            assert (uid == check_cnt);
            ++check_cnt;
        }
    } else {
        WERR << ": Query failed with " << qres.error << ".\n";
        exit(1);
    }

    ev->trigger();
}

void main2(CLOSURE);

tamed void main2() {
    tvars {
        ptr<mongo_connection_t> conn;
        vec<mongo_host_t> hosts();
    }
    hosts.push_back(mongo_host_t("bogus"));
    hosts.push_back(mongo_host_t("127.0.0.1"));
    conn = New refcounted<mongo_connection_t>(hosts, "mydb");
    twait { pub_mongo_test(conn, mkevent()); }
    twait { native_mongo_test(conn, mkevent()); }
    exit(0);
}

int main(int argc, char** argv) {
    make_sync(0);
    make_sync(1);
    make_sync(2);

    main2();
    amain();
    return 0;
}
