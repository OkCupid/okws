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
        // an out parameter. Internal json_to_xdr takes a ref to avoid
        // code duplication
        template<typename T> bool json_to_xdr(T* out_xdr) {
            return json_to_xdr(*out_xdr);
        }
        template<typename T> bool json_to_xdr(ptr<T> out_xdr) {
            return json_to_xdr(*out_xdr);
        }

        void set(bool error, str status, pub3::obj_t obj = pub3::obj_t()) {
            m_error = error; m_status = status; m_obj = obj;
        }
    private:
        template<typename T> bool json_to_xdr(T& out_xdr) {
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

struct redis_del_res_t {
    int64_t keys_removed;
    str error_msg;
    bool is_err() { return keys_removed == -1; }
};

class redisReply;
class redisAsyncContext;
typedef event<redis_res_t>::ref ev_redis_res_t;
typedef callback<void, redis_del_res_t>::ref cb_rdr;
typedef event<redis_del_res_t>::ref ev_redis_del_res_t;

//------------------------------------------------------------------------

// This is the tame redis client. Check out offline/redis_tester.T for an
// example of using it.
class RedisCli {
    public:
        RedisCli()
            : m_host(""), m_port(0), m_c(nullptr), m_bufsize(1024*16),
            m_connected(false), m_reconnecting(false), m_name("RedisCli")
        {}
        virtual ~RedisCli() { disconnect(); }

        void setReconnect(bool recon) { m_recon = recon; }
        void setBufSize(uint32_t size) { m_bufsize = size; }
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
        uint32_t m_bufsize;
        bool m_connected, m_reconnecting;
        str m_name;
        time_t m_timeout = 86400;
        bool m_recon = true;

        bhash<str> m_evalshas;
};

//-----------------------------------------------------------------------------

// Gotta do crc16
static const uint16_t crc16tab[256]= {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

//------------------------------------------------------------------------

typedef event<ptr<RedisCli>>::ref redis_ev_t;

typedef std::pair<str, uint16_t> node_t;
template<> struct hashfn<node_t> {
    hashfn () {}
    hash_t operator() (const node_t &b) const {
        return hashfn<str>()(b.first << b.second);
    }
};
template<> struct equals<node_t> {
    equals() {}
    bool operator() (const node_t &b1, const node_t &b2) const {
        return b1.second == b2.second && b1.first == b2.first;
    }
};

// This is the tamed redis cluster client implementation.
class RedisClusterCli {
    public:
        RedisClusterCli() : m_max_cached_connections(50), m_ttl(10), m_bufsize(1024*16) {}
        virtual ~RedisClusterCli() {
            m_max_cached_connections = 0;
            disconnect();
        }
        void disconnect();
        void connect(vec<node_t> startup_nodes, evb_t::ptr ev=nullptr, CLOSURE);
        void runCmd(std::initializer_list<str> l, ev_redis_res_t::ptr ev=nullptr);
        void runCmd(const vec<str> &cmds, ev_redis_res_t::ptr ev=nullptr,
                    CLOSURE);
        void runCmd(std::initializer_list<std::pair<const char*,size_t>> l,
                    ev_redis_res_t::ptr ev=nullptr, CLOSURE);
        void setBufSize(uint32_t size) { m_bufsize = size; }
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
    private:
        // Functions
        uint16_t assignKeyslot(const char* key, size_t key_len);
        void getConnFromSlot(uint16_t slot_id, redis_ev_t::ptr ev=nullptr, CLOSURE);
        void getRandomConn(redis_ev_t::ptr ev=nullptr, CLOSURE);
        void getSlotsCache(evb_t ev, CLOSURE);

        // Member variables
        uint16_t m_max_cached_connections;
        uint16_t m_ttl;
        uint32_t m_bufsize;
        qhash<node_t, ptr<RedisCli>> m_connection_cache;
        vec<node_t> m_startup_nodes;
        qhash<uint16_t, ptr<node_t>> m_slots;
        bool m_dirty_tables;
        qhash<node_t, bhash<str>> m_evalshas;
};

