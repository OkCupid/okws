#include "async.h"
#include "crypt.h"
#include "redis.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "tame_connectors.h"

typedef event<int, redisAsyncContext*>::ref redis_conn_res_ev;
typedef event<redisReply*>::ref ev_redis_reply_t;

int redisLibasyncAttach(redisAsyncContext* ac);
void gDisconnectHandler(const redisAsyncContext* c, int status);
void gConnectHandler(const redisAsyncContext* c, int status);
void gReplyHandler(redisAsyncContext* c, void* r, void* privdata);

//------------------------------------------------------------------------
// RedisDispatcher
//
// links up stateless function pointers that hiredis uses to callback and event
// objects that tame uses
//------------------------------------------------------------------------

class RedisDispatcher {
  public:
    RedisDispatcher() {}

    void
    addCli(const redisAsyncContext* c, RedisCli* cli) {
        m_cliMap.insert((uint64_t)c, cli);
    }
    void
    addConnEv(const redisAsyncContext* c, evi_t ev) {
        m_connEvMap.insert((uint64_t)c, ev);
    }
    void
    addReplyEv(uint64_t key, ev_redis_reply_t ev) {
        m_replyEvMap.insert(key, ev);
    }

    void
    removeCli(const redisAsyncContext* c) {
        m_cliMap.remove((uint64_t)c);
    }

    RedisCli* useCli(const redisAsyncContext* c);
    evi_t::ptr useConnEv(const redisAsyncContext* c);
    ev_redis_reply_t::ptr useReplyEv(uint64_t key);

  private:
    qhash<uint64_t, RedisCli*> m_cliMap;
    qhash<uint64_t, evi_t> m_connEvMap;
    qhash<uint64_t, ev_redis_reply_t> m_replyEvMap;
};

RedisDispatcher g_dispatcher;

#define rcwarn warn << m_name << "::" << __func__ << " "

//------------------------------------------------------------------------
// implementation of the RedisClusterCli class
//------------------------------------------------------------------------

uint32_t
crc16(const char* buf, int len) {
    int counter;
    uint32_t crc = 0;
    for (counter = 0; counter < len; counter++) {
        crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ *buf++) & 0x00FF];
    }
    return crc;
}

tamed void
RedisClusterCli::evalLua(
    const char* script,
    str ssha1,
    std::initializer_list<str> keys,
    std::initializer_list<str> args,
    ev_redis_res_t::ptr ev) {
    vec<str> vkeys(keys);
    vec<str> vargs(args);
    evalLua(script, ssha1, vkeys, vargs, ev);
}

tamed void
RedisClusterCli::evalLua(
    const char* script,
    str ssha1,
    const vec<str>& keys,
    const vec<str>& args,
    ev_redis_res_t::ptr ev) {

    vec<str> cmds;
    ptr<node_t> p_node = nullptr;
    uint16_t slot_id = assignKeyslot(keys[0].cstr(), keys[0].len());

    if (m_slots[slot_id]) {
        p_node = *m_slots[slot_id];
    }

    bhash<str>* evalshas = nullptr;

    if (p_node && !m_evalshas[*p_node]) {
        m_evalshas.insert(*p_node, {});
    }

    if (p_node) {
        evalshas = m_evalshas[*p_node];
    }

    if (evalshas && (*evalshas)[ssha1]) {
        cmds.push_back("EVALSHA");
        cmds.push_back(ssha1);
    } else {
        if (evalshas) {
            evalshas->insert(ssha1);
        }
        cmds.push_back("EVAL");
        cmds.push_back(script);
    }

    strbuf nkbuf;
    str nkstr;
    nkbuf << keys.size();
    nkstr = str(nkbuf);

    cmds.push_back(nkstr);
    for (auto k : keys) cmds.push_back(k);
    for (auto a : args) cmds.push_back(a);

    runCmd(cmds, ev);
}

tamed void
RedisClusterCli::getRandomConn(redis_ev_t::ptr ev) {
    tvars {
        ptr<node_t> p_node;
        bool conn_ok(true);
        redis_res_t res;
        ptr<RedisCli> rcli;
        size_t i;
    }
    for (i = 0; i < m_startup_nodes.size(); i++) {
        p_node = New refcounted<node_t>(m_startup_nodes[i]);
        rcli = new refcounted<RedisCli>(RedisCli());
        rcli->setReconnect(false);
        rcli->setBufSize(m_bufsize);
        twait {
            rcli->connect(p_node->first, p_node->second, mkevent(conn_ok));
        }
        if (conn_ok) {
            twait {
                rcli->runCmd({"PING"}, mkevent(res));
            }
            if (!res.is_err()) {
                ev->trigger(rcli);
                return;
            } else {
                rcli->disconnect();
            }
        } else {
            rcli->disconnect();
        }
    }
    ev->trigger(nullptr);
    return;
}

tamed void
RedisClusterCli::getConnFromSlot(uint16_t slot_id, redis_ev_t::ptr ev) {
    tvars {
        ptr<node_t> p_node;
        bool conn_ok;
        ptr<RedisCli> rcli;
    }

    p_node = *m_slots[slot_id];

    if (p_node == nullptr) {
        warn << "BWB_WARN: NO KNOWN NODE FOR " << slot_id << "\n";
        twait {
            getRandomConn(ev);
        }
    } else {
        if (m_connection_cache[*p_node] != nullptr) {
            rcli = *m_connection_cache[*p_node];
            if(rcli) {
                if (rcli->isConnected()) {
                    ev->trigger(rcli);
                    return;
                }
            }
        }

        rcli = new refcounted<RedisCli>(RedisCli());
        rcli->setReconnect(false);
        rcli->setBufSize(m_bufsize);
        twait {
            rcli->connect(p_node->first, p_node->second, mkevent(conn_ok));
        }

        if (conn_ok) {
            if (rcli->isConnected()) {
                m_connection_cache.insert(*p_node, rcli);
                ev->trigger(rcli);
                return;
            }
        }
    }
    twait {
        getRandomConn(ev);
    }
    return;
}

void
RedisClusterCli::disconnect() {
    for (node_t n : m_connection_cache) {
        if (m_connection_cache.size() <= m_max_cached_connections) {
            break;
        }
        (*m_connection_cache[n])->disconnect();
        m_connection_cache.remove(n);
    }
}

tamed void
RedisClusterCli::getSlotsCache(evb_t ev) {
    tvars {
        RedisCli cli;
        bool ok;
        size_t i;
        redis_res_t res;
    }

    for (i = 0; i < m_startup_nodes.size(); i++) {
        twait {
            cli.connect(
                m_startup_nodes[i].first,
                m_startup_nodes[i].second,
                mkevent(ok));
        }
        if (ok) {
            twait {
                cli.runCmd({"CLUSTER", "SLOTS"}, mkevent(res));
            }
            if (res.is_err()) {
                continue; // only freak out if no nodes are a success.
            } else if (res.obj().to_list()->size() == 0) {
                continue; // Not a member of the cluster yet
            } else {
                m_slots.clear();
                for (pub3::obj_t slot : *res.obj().to_list()) {
                    for (int slot_id = slot[0].to_int(); slot_id <= slot[1].to_int(); slot_id++) {
                        node_t node(slot[2][0].to_str(), slot[2][1].to_int());
                        m_slots.insert((uint16_t)slot_id, New refcounted<node_t>(node));
                    }
                }
                m_dirty_tables = false;
                ev->trigger(true);
                return;
            }
        }
    }

    // No nodes can get the slots cache. Have the startup clusters all moved?
    warn << "BWB_ERROR: RedisClusterCli could not find a single active node \n";
    ev->trigger(false);
    return;
}

tamed void
RedisClusterCli::connect(vec<node_t> startup_nodes, evb_t::ptr ev) {
    tvars {
        bool ok;
        size_t i;
    }
    for (i = 0; i < startup_nodes.size(); i++) {
        node_t node(startup_nodes[i].first, startup_nodes[i].second);
        m_startup_nodes.push_back(node);
    }
    twait {
        getSlotsCache(mkevent(ok));
    }
    ev->trigger(ok);
}

void
RedisClusterCli::runCmd(std::initializer_list<str> l, ev_redis_res_t::ptr ev) {
    vec<str> cmds(l);
    runCmd(cmds, ev);
}

tamed void
RedisClusterCli::runCmd(const vec<str>& cmds, ev_redis_res_t::ptr ev) {
    tvars {
        uint16_t keyslot;
        ptr<RedisCli> cli;
        redis_res_t res;
        vec<str> ncmds;
        bool random_node(false);
        bool asking(false);
        uint16_t ttl(32);
        int32_t slot;
        int32_t port;
        bool ok;
        vec<str> status_parts;
        strbuf cmdbuf;
        size_t key_index;
    }

    for (size_t i = 0; i < cmds.size(); i++) {
        cmdbuf << cmds[i] << " ";
        ncmds.push_back(cmds[i]);
    }
    if (m_dirty_tables) {
         twait {
            getSlotsCache(mkevent(ok));
        }
        if(!ok) {
            warn << "BWB_ERROR: Failed to refresh Redis Cluster slot cache. Unrecoverable. Redis might be out of space\n";
            res.set(true, "not connected"); //  true sets it to being an error.
            if (ev) {
                ev->trigger(res);
            }
            return;
        }
    }
    while (ttl > 0) {
        ttl--;
        if (!random_node) {
            // TJ: This is a gross hack. EVAL and EVALSHA have the first key as the
            // fourth arg instead of the second arg
            key_index = (ncmds[0] == "EVAL" || ncmds[0] == "EVALSHA") ? 3 : 1;
            keyslot = assignKeyslot(ncmds[key_index].cstr(), ncmds[key_index].len());
            twait {
                getConnFromSlot(keyslot, mkevent(cli));
            }
        } else {
            twait {
                getRandomConn(mkevent(cli));
            }
            if (cli) {
                random_node = false;
            }
        }
        if (cli) {
            if (asking) {
                asking = false;
                twait {
                    cli->runCmd({"ASKING"}, mkevent(res));
                }
            }
            twait {
                cli->runCmd(ncmds, mkevent(res));
            }

            if (res.is_err()) {
                split(&status_parts, rxx("[ :]"), res.status());
                res.set(false, "connected");
                if (status_parts[0] == "CLUSTERDOWN") {
                    res.set(true, "Cluster is Down"); //  true sets it to being an error.
                } else if (status_parts[0] == "ASK") {
                    // http://redis.io/commands/cluster-setslot
                    asking = true;
                } else if (status_parts[0] == "MOVED") {
                    m_dirty_tables = true;
                    status_parts[1].to_int32(&slot);
                    status_parts[3].to_int32(&port);
                    node_t n(status_parts[2], port);
                    m_slots.remove((uint16_t)slot);
                    m_slots.insert((uint16_t)slot, New refcounted<node_t>(n));
                } else {
                    random_node = true;
                }
            } else {
                ev->trigger(res);
                return;
            }
            twait {
                delaycb(1, 0, mkevent());
            }
        }
    }
    ev->trigger(res);
}

// keyslot description at http://redis.io/topics/cluster-spec
uint16_t
RedisClusterCli::assignKeyslot(const char* key, size_t keylen) {
    size_t s, e;
    for (s = 0; s < keylen; s++) {
        if (key[s] == '{') {
            break;
        }
    }
    if (s == keylen) {
        return crc16(key, keylen) & 16383;
    }
    for (e = s + 1; e < keylen; e++) {
        if (key[e] == '}') {
            break;
        }
    }
    if (e == keylen || e == s + 1) {
        return crc16(key, keylen) & 16383;
    }
    return crc16(key + s + 1, e - s - 1) & 16383;
}

//------------------------------------------------------------------------
// implementation of the RedisCli class
//------------------------------------------------------------------------

tamed void
RedisCli::connect(str host, uint port, evb_t::ptr ev) {
    tvars {
        int status;
    }
    if (isConnected()) {
        if (ev)
            ev->trigger(false);
        return;
    }
    m_host = host;
    m_port = port;
    m_c = redisAsyncConnectBindWithReuse(host.cstr(), port, "0.0.0.0");
    m_c->c.reader->maxbuf = m_bufsize;
    if (m_c->err) {
        rcwarn << "connect error: " << m_c->errstr << "\n";
        if(m_recon) {
            delaycb(5, 0, wrap(this, &RedisCli::reconnect));
        }
        if (ev) {
            ev->trigger(false);
        }
        return;
    }
    status = redisLibasyncAttach(m_c);
    if (status != REDIS_OK) {
        rcwarn << "async attach error: " << status << "\n";
        if(m_recon) {
            delaycb(5, 0, wrap(this, &RedisCli::reconnect));
        }
        if (ev) {
            ev->trigger(false);
        }
        return;
    }
    g_dispatcher.addCli(m_c, this);
    redisAsyncSetDisconnectCallback(m_c, &gDisconnectHandler);
    twait {
        g_dispatcher.addConnEv(m_c, mkevent(status));
        redisAsyncSetConnectCallback(m_c, &gConnectHandler);
    }
    if (status != REDIS_OK) {
        rcwarn << "callback error: " << m_c->errstr << "\n";
        if(m_recon) {
            delaycb(5, 0, wrap(this, &RedisCli::reconnect));
        }
        if (ev) {
            ev->trigger(false);
        }
        return;
    }
    m_connected = true;
    m_reconnecting = false;
    if (ev) {
        ev->trigger(true);
    }
}

//------------------------------------------------------------------------

void
RedisCli::runCmd(std::initializer_list<str> l, ev_redis_res_t::ptr ev) {
    vec<str> cmds(l);
    runCmd(cmds, ev);
}

//------------------------------------------------------------------------

tamed void
RedisCli::runCmd(const vec<str>& cmds, ev_redis_res_t::ptr ev) {
    vec<std::pair<const char*, size_t>> rcmds;
    rcmds.setsize(cmds.size());
    for (size_t i = 0; i < cmds.size(); i++) {
        rcmds[i] = std::make_pair(cmds[i].cstr(), cmds[i].len());
    }
    runCmd(rcmds, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::runCmd(
    std::initializer_list<std::pair<const char*, size_t>> l,
    ev_redis_res_t::ptr ev) {
    vec<std::pair<const char*, size_t>> cmds(l);
    runCmd(cmds, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::runCmd(
    const vec<std::pair<const char*, size_t>>& cmds, ev_redis_res_t::ptr ev) {

    tvars {
        redisCallbackFn* fn(nullptr);
        uint64_t data(0);
        size_t n;
        vec<const char*> ccharcmds;
        vec<size_t> lens;
        redisReply* r(nullptr);
        redis_res_t res;
        pub3::obj_t res_obj;
        outcome_t outcome;
    }
    if (!isConnected()) {
        res.set(true, "not connected");
        if (ev) {
            ev->trigger(res);
        }
        return;
    }
    n = cmds.size();
    ccharcmds.reserve(n);
    lens.reserve(n);
    for (auto s : cmds) {
        ccharcmds.push_back(s.first);
        lens.push_back(s.second);
    }

    twait {
        if (ev) {
            fn = gReplyHandler;
            data = rnd.gethyper();
            g_dispatcher.addReplyEv(
                data, connector::timeout(mkevent(r), m_timeout, 0, &outcome));
        }
        redisAsyncCommandArgv(
            m_c, fn, (void*)data, n, ccharcmds.begin(), lens.begin());
    }
    if (ev == nullptr) {
        return;
    }

    if (outcome == OUTCOME_TIMEDOUT) {
        res.set(true, "timed out");
        ev->trigger(res);
        return;
    }

    if (r == nullptr) {
        res.set(true, "null reply");
        ev->trigger(res);
        return;
    }
    if (r->type == REDIS_REPLY_ERROR || r->type == REDIS_REPLY_STATUS) {
        res.set(r->type == REDIS_REPLY_ERROR, str(r->str, r->len));
        ev->trigger(res);
        return;
    }
    res_obj = parseReply(r);
    res.set(false, nullptr, res_obj);
    ev->trigger(res);

    // r is freed by hiredis when this function returns
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::evalLua(
    const char* script,
    str ssha1,
    std::initializer_list<str> keys,
    std::initializer_list<str> args,
    ev_redis_res_t::ptr ev) {
    vec<str> vkeys(keys);
    vec<str> vargs(args);
    evalLua(script, ssha1, vkeys, vargs, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::evalLua(
    const char* script,
    str ssha1,
    const vec<str>& keys,
    const vec<str>& args,
    ev_redis_res_t::ptr ev) {
    vec<std::pair<const char*, size_t>> vkeys;
    vec<std::pair<const char*, size_t>> vargs;
    vkeys.setsize(keys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        vkeys[i] = std::make_pair(keys[i].cstr(), keys[i].len());
    }
    vargs.setsize(args.size());
    for (size_t i = 0; i < args.size(); i++) {
        vargs[i] = std::make_pair(args[i].cstr(), args[i].len());
    }
    evalLua(script, ssha1, vkeys, vargs, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::evalLua(
    const char* script,
    str ssha1,
    std::initializer_list<std::pair<const char*, size_t>> keys,
    std::initializer_list<std::pair<const char*, size_t>> args,
    ev_redis_res_t::ptr ev) {
    vec<std::pair<const char*, size_t>> vkeys(keys);
    vec<std::pair<const char*, size_t>> vargs(args);
    evalLua(script, ssha1, vkeys, vargs, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::evalLua(
    const char* script,
    str ssha1,
    const vec<std::pair<const char*, size_t>>& keys,
    const vec<std::pair<const char*, size_t>>& args,
    ev_redis_res_t::ptr ev) {

    vec<std::pair<const char*, size_t>> cmds;
    if (m_evalshas[ssha1]) {
        cmds.push_back(std::make_pair("EVALSHA", 7));
        cmds.push_back(std::make_pair(ssha1.cstr(), ssha1.len()));
    } else {
        m_evalshas.insert(ssha1);
        cmds.push_back(std::make_pair("EVAL", 4));
        cmds.push_back(std::make_pair(script, strlen(script)));
    }

    strbuf nkbuf;
    str nkstr;
    nkbuf << keys.size();
    nkstr = str(nkbuf);

    cmds.push_back(std::make_pair(nkstr.cstr(), nkstr.len()));
    for (auto k : keys) cmds.push_back(k);
    for (auto a : args) cmds.push_back(a);

    runCmd(cmds, ev);
}

//------------------------------------------------------------------------

void
RedisCli::disconnect() {
    if (m_connected && m_c->err == 0) {
        m_connected = false;
        redisAsyncDisconnect(m_c);
    } else {
        g_dispatcher.removeCli(m_c);
        m_c = nullptr;
        m_connected = false;
    }
}

//------------------------------------------------------------------------

void
RedisCli::disconnectHandler(int status) {
    if (status != REDIS_OK) {
        rcwarn << "error: " << m_c->errstr << "\n";
        if (m_recon) {
            reconnect();
            return;
        }
    }
    // no need to free m_c on successful disconnect, it will already have
    // been freed
    if (m_c) {
        g_dispatcher.removeCli(m_c);
        m_connected = false;
        m_c = nullptr;
    }
}

//------------------------------------------------------------------------

void
RedisCli::reconnect() {
    if (m_recon) {
        m_reconnecting = true;
        connect(m_host, m_port);
    }
}

//------------------------------------------------------------------------

pub3::obj_t
RedisCli::parseReply(redisReply* r) {
    assert(r);
    pub3::obj_t res;
    switch (r->type) {
    case REDIS_REPLY_NIL:
        break;
    case REDIS_REPLY_INTEGER:
        res = (int64_t)r->integer;
        break;
    case REDIS_REPLY_STRING:
        res = str(r->str, r->len);
        break;
    case REDIS_REPLY_ARRAY: {
        for (size_t i = 0; i < r->elements; i++) {
            res.push_back(parseReply(r->element[i]));
        }
    } break;
    case REDIS_REPLY_STATUS:
        res = str(r->str, r->len);
        break;
    case REDIS_REPLY_ERROR:
        res = str(r->str, r->len);
        break;
    default:
        warn << "MM_ERROR: unknown Redis reply, dying!: reply: " << r->type
             << "\n";
        assert(false);
    }
    return res;
}

//------------------------------------------------------------------------
// Implementation of the RedisDispatcher
//------------------------------------------------------------------------

RedisCli*
RedisDispatcher::useCli(const redisAsyncContext* c) {
    uint64_t key = (uint64_t)c;
    RedisCli** cli = m_cliMap[key];
    if (!cli) {
        return nullptr;
    }
    m_cliMap.remove(key);
    return *cli;
}

evi_t::ptr
RedisDispatcher::useConnEv(const redisAsyncContext* c) {
    uint64_t key = (uint64_t)c;
    evi_t::ptr ev = m_connEvMap[(uint64_t)c];
    m_connEvMap.remove(key);
    return ev;
}

ev_redis_reply_t::ptr
RedisDispatcher::useReplyEv(uint64_t key) {
    ev_redis_reply_t::ptr ev = m_replyEvMap[key];
    m_replyEvMap.remove(key);
    return ev;
}

void
gDisconnectHandler(const redisAsyncContext* c, int status) {
    RedisCli* cli = g_dispatcher.useCli(c);
    if (cli) {
        cli->disconnectHandler(status);
    }
}

void
gConnectHandler(const redisAsyncContext* c, int status) {
    evi_t::ptr ev = g_dispatcher.useConnEv(c);
    assert(ev);
    ev->trigger(status);
}

void
gReplyHandler(redisAsyncContext* c, void* r, void* privdata) {
    uint64_t key = (uint64_t)privdata;
    ev_redis_reply_t::ptr ev = g_dispatcher.useReplyEv(key);
    if (ev) {
        ev->trigger((redisReply*)r);
    }
    // r is freed by the redis library when this function returns.
    // the code executed by ev->trigger MUST copy r before calling twait
}

//------------------------------------------------------------------------
// hiredis / libasync adapter logic
//------------------------------------------------------------------------

struct redisLibasyncEvents {
    redisAsyncContext* context;
};

void
redisLibasyncReadEvent(redisLibasyncEvents* e) {
    redisAsyncHandleRead(e->context);
}

void
redisLibasyncWriteEvent(redisLibasyncEvents* e) {
    redisAsyncHandleWrite(e->context);
}

void
redisLibasyncAddRead(void* privdata) {
    redisLibasyncEvents* e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selread, wrap(redisLibasyncReadEvent, e));
}

void
redisLibasyncDelRead(void* privdata) {
    redisLibasyncEvents* e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selread, nullptr);
}

void
redisLibasyncAddWrite(void* privdata) {
    redisLibasyncEvents* e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selwrite, wrap(redisLibasyncWriteEvent, e));
}

void
redisLibasyncDelWrite(void* privdata) {
    redisLibasyncEvents* e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selwrite, nullptr);
}

void
redisLibasyncCleanup(void* privdata) {
    redisLibasyncEvents* e = (redisLibasyncEvents*)privdata;
    redisLibasyncDelRead(privdata);
    redisLibasyncDelWrite(privdata);
    delete e;
}

int
redisLibasyncAttach(redisAsyncContext* ac) {
    redisLibasyncEvents* e;

    /* Nothing should be attached when something is already attached */
    if (ac->ev.data != NULL)
        return REDIS_ERR;

    /* Create container for context */
    e = New redisLibasyncEvents();
    e->context = ac;

    /* Register functions to start/stop listening for events */
    ac->ev.addRead = redisLibasyncAddRead;
    ac->ev.delRead = redisLibasyncDelRead;
    ac->ev.addWrite = redisLibasyncAddWrite;
    ac->ev.delWrite = redisLibasyncDelWrite;
    ac->ev.cleanup = redisLibasyncCleanup;
    ac->ev.data = e;

    return REDIS_OK;
}
