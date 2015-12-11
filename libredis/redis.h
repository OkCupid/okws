#pragma once

#include "async.h"
#include "pub3obj.h"
#include "json_rpc.h"

// This is returned as the result of running a redis command
class redis_res_t {
    public:
        bool is_err() { return m_error; }
        str status() { return m_status; }
        pub3::obj_t obj() { return m_obj; }
        ptr<pub3::expr_t> to_json() {
            if (!m_parsed) {
                m_parsed = pub3::json_parser_t::parse(obj().to_str());
            }

            return m_parsed;
        }

        // TJ: API should take a pointer/ptr to be clear at callsite it's
        // an out parameter. Internal try_parse_xdr takes a ref to avoid
        // code duplication
        template<typename T> bool try_parse_xdr(T* out_xdr) {
            return try_parse_xdr(*out_xdr);
        }
        template<typename T> bool try_parse_xdr(ptr<T> out_xdr) {
            return try_parse_xdr(*out_xdr);
        }

        void set(bool error, str status, pub3::obj_t obj = pub3::obj_t()) {
            m_error = error; m_status = status; m_obj = obj;
        }
    private:
        template<typename T> bool try_parse_xdr(T& out_xdr) {
            if (m_error || m_obj.is_null()) {
                return false;
            }
            return json2xdr(out_xdr, to_json());
        }
        bool m_error;
        str m_status;
        pub3::obj_t m_obj;
        ptr<pub3::expr_t> m_parsed;
};

class redisReply;
class redisAsyncContext;
typedef event<redis_res_t>::ref ev_redis_res_t;

//------------------------------------------------------------------------

// This is the tame redis client. Check out offline/redis_tester.T for an
// example of using it.
class RedisCli {
    public:
        RedisCli()
            : m_host(""), m_port(0), m_c(nullptr) , m_connected(false),
            m_reconnecting(false), m_name("RedisCli")
        {}
        virtual ~RedisCli() { disconnect(); }

        void setReconnect(bool recon) { m_recon = recon; }
        void setTimeout(time_t to) { m_timeout = to; }

        void connect(str host, uint port, evb_t::ptr ev=nullptr, CLOSURE);
        bool isConnected() const { return m_connected && !m_reconnecting; }
        void runCmd(std::initializer_list<str> l, ev_redis_res_t::ptr ev=nullptr);
        void runCmd(const vec<str> &cmds, ev_redis_res_t::ptr ev=nullptr,
                    CLOSURE);
        void runCmd(std::initializer_list<std::pair<const char*,size_t>> l,
                    ev_redis_res_t::ptr ev=nullptr, CLOSURE);
        void runCmd(const vec<std::pair<const char*,size_t>>& cmds,
                    ev_redis_res_t::ptr ev=nullptr, CLOSURE);
        void runTransaction(
            std::initializer_list<std::initializer_list<str>> cmds,
            ev_redis_res_t::ptr ev = nullptr
        ) {
            runCmd({ "MULTI" });
            for (const auto &cmd : cmds) { runCmd(cmd); }
            runCmd({ "EXEC" }, ev);
        }

        void evalLua(const char* script, str ssha1,
                     std::initializer_list<str> keys,
                     std::initializer_list<str> args, 
                     ev_redis_res_t::ptr ev=nullptr , CLOSURE);
        void evalLua(const char* script, str ssha1, 
                     const vec<str>& keys,
                     const vec<str>& args, 
                     ev_redis_res_t::ptr ev=nullptr, CLOSURE);
        void evalLua(const char* script, str ssha1,
                     std::initializer_list<std::pair<const char*,size_t>> keys,
                     std::initializer_list<std::pair<const char*,size_t>> args, 
                     ev_redis_res_t::ptr ev=nullptr , CLOSURE);
        void evalLua(const char* script, str ssha1, 
                     const vec<std::pair<const char*,size_t>>& keys,
                     const vec<std::pair<const char*,size_t>>& args, 
                     ev_redis_res_t::ptr ev=nullptr, CLOSURE);

        void disconnect();

        // I wish this could be private: don't use!
        void disconnectHandler(int status);
    private:
        void reconnect();
        pub3::obj_t parseReply(redisReply *r);

        str m_host;
        uint m_port;
        redisAsyncContext *m_c;
        bool m_connected, m_reconnecting;
        str m_name;
        time_t m_timeout = 86400;
        bool m_recon = true;

        bhash<str> m_evalshas;
};

//-----------------------------------------------------------------------------


