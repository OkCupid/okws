// -*- mode:c++ -*-
#include "amongo.h"
#include "tame_io.h" // tame::wait{read, write}
#include "async.h"   // tcpconnect
#include "amisc.h"   // arandom
#include "abson.h"
#include "mongo.h"

#include <cstring>

extern "C" {
#include <time.h>
}

#define SHORT_FILE \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define ERRLINE \
    warn << SHORT_FILE << "[" << __LINE__ << "] in " << __func__ << ": "

#define PUNIX_ERROR                                        \
    do {                                                   \
        int err = errno;                                   \
        ERRLINE << "unix error " << strerror(err) << "\n"; \
    } while (0)

namespace {
class QueryRespReader : public AVRAdapter<bson_expr_reader_engine<
                                okmongo::ResponseReader<QueryRespReader>>> {
    ptr<pub3::expr_list_t> lst_;

public:
    explicit QueryRespReader(
            ptr<pub3::expr_list_t> v = pub3::expr_list_t::alloc())
        : lst_(v) {}

    void EmitStart(const okmongo::ResponseHeader &hdr) {
        if (hdr.number_returned > 0) {
            lst_->reserve(lst_->size() + hdr.number_returned);
        }
    }

    void EmitDocumentDone() {
        if (!error_) {
            lst_->push_back(res_);
        }
        res_ = nullptr;
    }

    ptr<pub3::expr_list_t> get_res() {
        if (state_ != State::kDone) {
            return nullptr;
        }
        return lst_;
    }
};

struct timespec get_monotonic_time() {
    struct timespec res;
#ifndef _POSIX_TIMERS
#error "Your os doesn't support clock_get_time..."
#endif
#ifdef CLOCK_MONOTONIC_COARSE
    int ok = clock_gettime(CLOCK_MONOTONIC_COARSE, &res);
#elif defined _POSIX_MONOTONIC_CLOCK
    int ok = clock_gettime(CLOCK_MONOTONIC, &res);
#else
    int ok = clock_gettime(CLOCK_REALTIME, &res);
#endif
    if (ok == -1) {
        res.tv_sec = 0;
        res.tv_nsec = 0;
    }
    return res;
}

 struct timespec get_future_time(int ms) {
     if (ms <= 0) {
         struct timespec res;
         res.tv_sec = std::numeric_limits<time_t>::max();
         res.tv_nsec = 999999999;
         return res;
     }
     time_t tv_sec = ms / 1000;
     long tv_nsec = (ms % 1000) * 1000000;
     struct timespec res = get_monotonic_time();
     res.tv_nsec += tv_nsec;
     if (res.tv_nsec >= 1000000000) {
         res.tv_nsec -= 1000000000;
         res.tv_sec += 1;
     }
     res.tv_sec += tv_sec;
     return res;
}

const struct timespec &min(const struct timespec &ts1,
                           const struct timespec &ts2) {
    if (ts1.tv_sec < ts2.tv_sec || (ts1.tv_sec == ts2.tv_sec &&
                                    ts1.tv_nsec < ts2.tv_nsec)) {
        return ts1;
    }
    return ts2;
}

timecb_t *wait_until(const struct timespec &expiration, evv_t ev) {
    struct timespec now = get_monotonic_time();
    long tv_nsec = 0;
    time_t tv_sec = 0;
    if (now.tv_sec > expiration.tv_sec || (now.tv_sec == expiration.tv_sec &&
                                           now.tv_nsec >= expiration.tv_nsec)) {
        // already timedout
    } else if (now.tv_nsec > expiration.tv_nsec) {
        tv_sec = expiration.tv_sec - now.tv_sec - 1;
        tv_nsec = expiration.tv_nsec - now.tv_nsec + 1000000000;
    } else {
        tv_sec = expiration.tv_sec - now.tv_sec;
        tv_nsec = expiration.tv_nsec - now.tv_nsec;
    }
    return delaycb(tv_sec, tv_nsec, ev);
}

void timed_waitread(int fd, const struct timespec& exp, evb_t, CLOSURE);
void timed_waitwrite(int fd, const struct timespec& exp, evb_t, CLOSURE);
void timed_tcpconnect(str hostname, u_int16_t port, const struct timespec &exp,
                      evi_t ev, CLOSURE);
void timed_tcpconnect(const mongo_connection_fields_base_t &mc,
                      const struct timespec &exp, evi_t ev, CLOSURE);

tamed void timed_waitread(int fd, const struct timespec& exp, evb_t ev) {
    tvars {
        rendezvous_t<bool> rv(__FILE__, __LINE__);
        bool ok;
        timecb_t *tcb;
    }
    tcb = wait_until(exp, mkevent(rv, false));
    tame::waitread(fd, mkevent(rv, true));
    twait(rv, ok);
    if (!ok) {
        tame::clearread(fd);
    } else {
        timecb_remove(tcb);
    }
    ev->trigger(ok);
}

tamed void timed_waitwrite(int fd, const struct timespec& exp, evb_t ev) {
    tvars {
        rendezvous_t<bool> rv(__FILE__, __LINE__);
        bool ok;
        timecb_t *tcb;
    }
    tcb = wait_until(exp, mkevent(rv, false));
    tame::waitwrite(fd, mkevent(rv, true));
    twait(rv, ok);
    if (!ok) {
        tame::clearwrite(fd);
    } else {
        timecb_remove(tcb);
    }
    ev->trigger(ok);
}

tamed void timed_tcpconnect(str hostname, u_int16_t port,
                            const struct timespec &exp, evi_t ev) {
    tvars {
        rendezvous_t<bool> rv(__FILE__, __LINE__);
        bool ok;
        tcpconnect_t *tcp;
        timecb_t *tcb;
    }
    tcb = wait_until(exp, mkevent(rv, false));
    tcp = tcpconnect(hostname, port, mkevent_rs(ev->slot_set (), rv, true));
    twait(rv, ok);
    if (!ok) {
        tcpconnect_cancel(tcp);
        ev->trigger(-1);
    } else {
        timecb_remove(tcb);
        ev->trigger_no_assign();
    }
}

tamed void timed_tcpconnect(const mongo_connection_fields_base_t& mc,
                            const struct timespec &exp, evi_t ev) {
    tvars {
        int res(-1);
        size_t cnt(0);
        size_t idx(0);
        size_t offset;
        struct timespec ts;
    }
    if (mc.hosts_.size() == 0) {
        warn << "Mongo: cannot connect with an empty host list.\n";
        return ev->trigger(-1);
    }
    offset = arandom() % mc.hosts_.size();
    while (res < 0 && cnt < mc.hosts_.size()) {
        idx = (cnt + offset) % mc.hosts_.size();
        ts = get_future_time(mc.connect_timeout_);
        twait {
            timed_tcpconnect(mc.hosts_[idx].hostname,
                             mc.hosts_[idx].port,
                             min(exp, ts),
                             mkevent(res));
        }
        cnt++;
    }
    ev->trigger(res);
}
}

namespace okmongo {
template <>
bool BsonWriteFields<ptr<pub3::expr_dict_t>>(
        BsonWriter *w, ptr<pub3::expr_dict_t> const &dict) {
    if (!dict) {
        return false;
    }
    return pub_fields(w, *dict);
}
}

mongo_connection_t::mongo_connection_t(str hostname, str db, uint16_t port) {
    hosts_.push_back(mongo_host_t(hostname, port));
    db_ = db;
    socket_ = 0;
    requestid_ = 0;
}

mongo_connection_t::mongo_connection_t(const vec<mongo_host_t> &hosts, str db,
                                       int timeout, int connect_timeout) {
    hosts_ = hosts;
    db_ = db;
    socket_ = 0;
    requestid_ = 0;
    timeout_ = timeout;
    connect_timeout_ = connect_timeout_;
}

tamed void mongo_connection_base_t::run(const okmongo::BsonWriter &w,
                                        AbstractValueReader *r, evv_t ev) {
    tvars {
        const char *data(w.data());
        size_t len(w.len());
        struct timespec exp;
        bool ok;
    }
    exp = get_future_time(timeout_);
    twait { lock_.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }

    if (!socket_) {
        twait {
            timed_tcpconnect(*this, exp, mkevent(socket_));
        }
        if (socket_ <= 0) {
            socket_ = 0;
            r->AVRReportError("Failed to connect");
        }
    }

    while (len > 0 && !r->AVRIsDone()) {
        twait { timed_waitwrite(socket_, exp, mkevent(ok)); }
        if (!ok) {
            r->AVRReportError("Timeout while sending");
            abort();
            break;
        }
        ssize_t res = ::write(socket_, data, len);
        switch (res) {
        case -1:
            if (errno == EINTR) {
                continue;
            }
            r->AVRReportError(strbuf("Unix error while sending: %s",
                                     strerror(errno)));
            abort();
            break;
        case 0:
            r->AVRReportError("Connection closed while sending");
            abort();
            break;
        default:
            len -= static_cast<size_t>(res);
            data += res;
        }
    }

    while (!r->AVRIsDone()) {
        twait { timed_waitread(socket_, exp, mkevent(ok)); }
        if (!ok) {
            r->AVRReportError("Timeout while reading");
            abort();
            break;
        }
        ssize_t res = ::read(socket_, scratch_, kScratchLen);
        switch (res) {
            case -1:
                if (errno == EINTR) {
                    continue;
                }
                r->AVRReportError(strbuf("Unix error while reading: %s",
                                         strerror(errno)));
                abort();
                break;
            case 0:
                r->AVRReportError("Connection closed while reading");
                abort();
                break;
            default:
                int32_t consumed = r->AVRConsume(scratch_, res);
                if (consumed != res) {
                    r->AVRReportError("Number of bytes read did not match the "
                                      "number we got on the wire");
                    abort();
                }
        }
    }
    lock_.release();
    ev->trigger();
}

void mongo_connection_base_t::close() {
    if (socket_ == 0) {
        return;
    }
    int res = ::close(socket_);
    // Do not retry after a close!
    if (res == -1 && errno != EINTR) {
        PUNIX_ERROR;
    }
    socket_ = 0;
    requestid_ = 0;
}

void mongo_connection_base_t::abort() {
    close();
}

void mongo_connection_t::close() {
    twait { lock_.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }
    mongo_connection_t::close();
    lock_.release();
}

tamed void mongo_connection_t::query(str collection,
                                     const ptr<pub3::expr_dict_t> qry,
                                     pub3::xlev_t ev) {
    tvars {
        okmongo::BsonWriter out;
        QueryRespReader r;
    }
    okmongo::FillQueryOp(&out, ++requestid_, db_.cstr(), collection.cstr(),
                         qry);
    twait {
        run(out, &r, mkevent());
    }
    ev->trigger(r.get_res());
}

tamed void mongo_connection_base_t::run_cmd(const okmongo::BsonWriter &cmd,
                                            evb_t ev) {
    tvars {
        AVRAdapter<okmongo::OpResponseParser> r;
    }
    twait {
        run(cmd, &r, mkevent());
    }
    auto &res = r.Result();
    for (auto &e : res.errors) {
        ERRLINE << "Mongo error: " << e.msg.c_str() << "\n";
    }
    if (!res.ok) {
        warn << "Mongo error: Not ok " << res.ok << "\n";
    }
    ev->trigger(res.ok && res.errors.empty());
}

tamed void mongo_connection_base_t::run_cmd(ref<const okmongo::BsonWriter> cmd,
                                            evb_t ev) {
    tvars {
        bool ok(true);
    }
    twait { run_cmd(*cmd, mkevent(ok)); }
    ev->trigger(ok);
}
