#include "async.h"
#include "crypt.h"
#include "redis.h"
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "tame_connectors.h"

typedef event<int, redisAsyncContext*>::ref redis_conn_res_ev;
typedef event<redisReply*>::ref ev_redis_reply_t;

int redisLibasyncAttach(redisAsyncContext *ac);
void gDisconnectHandler(const redisAsyncContext *c, int status);
void gConnectHandler(const redisAsyncContext *c, int status);
void gReplyHandler(redisAsyncContext *c, void *r, void *privdata);

//------------------------------------------------------------------------
// RedisDispatcher
//
// links up stateless function pointers that hiredis uses to callback and event
// objects that tame uses
//------------------------------------------------------------------------


class RedisDispatcher {
    public:
        RedisDispatcher() {}

        void addCli(const redisAsyncContext *c, RedisCli *cli)
            { m_cliMap.insert((uint64_t)c, cli); }
        void addConnEv(const redisAsyncContext *c, evi_t ev)
            { m_connEvMap.insert((uint64_t)c, ev); }
        void addReplyEv(uint64_t key, ev_redis_reply_t ev)
            { m_replyEvMap.insert(key, ev); }

        void removeCli(const redisAsyncContext* c) {
            m_cliMap.remove((uint64_t)c);
        }

        RedisCli *useCli(const redisAsyncContext *c);
        evi_t::ptr useConnEv(const redisAsyncContext *c);
        ev_redis_reply_t::ptr useReplyEv(uint64_t key);

    private:
        qhash<uint64_t, RedisCli *> m_cliMap;
        qhash<uint64_t, evi_t> m_connEvMap;
        qhash<uint64_t, ev_redis_reply_t> m_replyEvMap;
};

RedisDispatcher g_dispatcher;

//------------------------------------------------------------------------
// implementation of the RedisCli class
//------------------------------------------------------------------------

#define rcwarn warn << m_name << "::" << __func__ << " "

tamed
void RedisCli::connect(str host, uint port, evb_t::ptr ev)
{
    tvars {
        int status;
    }
    if (isConnected()) {
        if(ev) ev->trigger(false);
        return;
    }
    rcwarn << "connecting to " << host << ":" << port << "\n";
    m_host = host;
    m_port = port;
    m_c = redisAsyncConnectBindWithReuse(host.cstr(), port, "0.0.0.0");
    if (m_c->err) {
        rcwarn << "connect error: " << m_c->errstr << "\n";
        delaycb(5, 0, wrap(this, &RedisCli::reconnect));
        if(ev) ev->trigger(false);
        return;
    }
    status = redisLibasyncAttach(m_c);
    if (status != REDIS_OK) {
        rcwarn << "async attach error: " << status << "\n";
        delaycb(5, 0, wrap(this, &RedisCli::reconnect));
        if(ev) ev->trigger(false);
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
        delaycb(5, 0, wrap(this, &RedisCli::reconnect));
        if(ev) ev->trigger(false);
        return;
    }
    m_connected = true;
    m_reconnecting = false;
    rcwarn << "connected to " << host << ":" << port << "\n";
    if(ev) ev->trigger(true);
}

//------------------------------------------------------------------------

void RedisCli::runCmd(std::initializer_list<str> l, ev_redis_res_t::ptr ev) {
    vec<str> cmds(l);
    runCmd(cmds, ev);
}

//------------------------------------------------------------------------

tamed
void RedisCli::runCmd(const vec<str> &cmds, ev_redis_res_t::ptr ev)
{
    vec<std::pair<const char*,size_t>> rcmds;
    rcmds.setsize(cmds.size());
    for (size_t i = 0; i < cmds.size(); i++) {
        rcmds[i] = std::make_pair(cmds[i].cstr(), cmds[i].len());
    }
    runCmd(rcmds, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::runCmd(std::initializer_list<std::pair<const char*,size_t>> l,
                 ev_redis_res_t::ptr ev) {
    vec<std::pair<const char*,size_t>> cmds(l);
    runCmd(cmds, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::runCmd(const vec<std::pair<const char*,size_t>>& cmds,
                 ev_redis_res_t::ptr ev) {

    tvars {
        redisCallbackFn *fn(nullptr);
        uint64_t data(0);
        size_t n;
        vec<const char *> ccharcmds;
        vec<size_t> lens;
        redisReply* r(nullptr);
        redis_res_t res;
        pub3::obj_t res_obj;
        outcome_t outcome;
    }
    if (!isConnected()) {
        res.set(true, "not connected");
        if (ev) { ev->trigger(res); }
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
            g_dispatcher.addReplyEv(data, connector::timeout(mkevent(r), 
                                    m_timeout, 0, &outcome));
        }
        redisAsyncCommandArgv(m_c, fn, (void *)data, n, ccharcmds.begin(), 
                              lens.begin());
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
RedisCli::evalLua(const char* script, str ssha1,
                  std::initializer_list<str> keys,
                  std::initializer_list<str> args,
                  ev_redis_res_t::ptr ev) {
    vec<str> vkeys(keys);
    vec<str> vargs(args);
    evalLua(script, ssha1, vkeys, vargs, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::evalLua(const char* script, str ssha1, const vec<str>& keys,
                  const vec<str>& args, ev_redis_res_t::ptr ev) {
    vec<std::pair<const char*,size_t>> vkeys;
    vec<std::pair<const char*,size_t>> vargs;
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
RedisCli::evalLua(const char* script, str ssha1,
                  std::initializer_list<std::pair<const char*,size_t>> keys,
                  std::initializer_list<std::pair<const char*,size_t>> args, 
                  ev_redis_res_t::ptr ev) {
    vec<std::pair<const char*,size_t>> vkeys(keys);
    vec<std::pair<const char*,size_t>> vargs(args);
    evalLua(script, ssha1, vkeys, vargs, ev);
}

//-----------------------------------------------------------------------------

tamed void
RedisCli::evalLua(const char* script, str ssha1, 
                  const vec<std::pair<const char*,size_t>>& keys,
                  const vec<std::pair<const char*,size_t>>& args, 
                  ev_redis_res_t::ptr ev) {

    vec<std::pair<const char*,size_t>> cmds;
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
    for (auto k : keys)
        cmds.push_back(k);
    for (auto a : args)
        cmds.push_back(a);

    runCmd(cmds, ev);
}

//------------------------------------------------------------------------

void RedisCli::disconnect() {
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

void RedisCli::disconnectHandler(int status) {
    if (status != REDIS_OK) {
        rcwarn << "error: " << m_c->errstr << "\n";
        if (m_recon) { reconnect(); return; }
    }
    // no need to free m_c on successful disconnect, it will already have
    // been freed
    g_dispatcher.removeCli(m_c);

    m_connected = false;
    m_c = nullptr;
    rcwarn << "disconnected from " << m_host << ":" << m_port << "\n";
}

//------------------------------------------------------------------------

void RedisCli::reconnect() {
    m_reconnecting = true;
    connect(m_host, m_port);
}

//------------------------------------------------------------------------

pub3::obj_t RedisCli::parseReply(redisReply *r) {
    assert(r);
    pub3::obj_t res;
    switch(r->type) {
        case REDIS_REPLY_NIL:
            break;
        case REDIS_REPLY_INTEGER:
            res = (int64_t)r->integer;
            break;
        case REDIS_REPLY_STRING:
            res = str(r->str, r->len);
            break;
        case REDIS_REPLY_ARRAY:
            {
                for (size_t i=0; i<r->elements; i++) {
                    res.push_back(parseReply(r->element[i]));
                }
            }
            break;
        case REDIS_REPLY_STATUS:
            res = str(r->str, r->len);
            break; 
        case REDIS_REPLY_ERROR:
            res = str(r->str, r->len);
            break; 
        default:
            warn << "MM_ERROR: unknown Redis reply, dying!: reply: " 
                 << r->type << "\n";
            assert(false);
    }
    return res;
}

//------------------------------------------------------------------------
// Implementation of the RedisDispatcher
//------------------------------------------------------------------------

RedisCli *
RedisDispatcher::useCli(const redisAsyncContext *c) {
    uint64_t key = (uint64_t)c;
    RedisCli **cli = m_cliMap[key];
    if (!cli) {
        return nullptr;
    }
    m_cliMap.remove(key);
    return *cli;
}

evi_t::ptr
RedisDispatcher::useConnEv(const redisAsyncContext *c) {
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

void gDisconnectHandler(const redisAsyncContext *c, int status) {
    RedisCli *cli = g_dispatcher.useCli(c);
    assert(cli);
    cli->disconnectHandler(status);
}

void gConnectHandler(const redisAsyncContext *c, int status) {
    evi_t::ptr ev = g_dispatcher.useConnEv(c);
    assert(ev);
    ev->trigger(status);
}

void gReplyHandler(redisAsyncContext *c, void *r, void *privdata) {
    uint64_t key = (uint64_t)privdata;
    ev_redis_reply_t::ptr ev = g_dispatcher.useReplyEv(key);
    if (ev) { ev->trigger((redisReply*)r); }
    // r is freed by the redis library when this function returns.
    // the code executed by ev->trigger MUST copy r before calling twait
}

//------------------------------------------------------------------------
// hiredis / libasync adapter logic
//------------------------------------------------------------------------


struct redisLibasyncEvents {
    redisAsyncContext *context;
};

void redisLibasyncReadEvent(redisLibasyncEvents *e) {
    redisAsyncHandleRead(e->context);
}

void redisLibasyncWriteEvent(redisLibasyncEvents *e) {
    redisAsyncHandleWrite(e->context);
}

void redisLibasyncAddRead(void *privdata) {
    redisLibasyncEvents *e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selread, wrap(redisLibasyncReadEvent, e));
}

void redisLibasyncDelRead(void *privdata) {
    redisLibasyncEvents *e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selread, nullptr);
}

void redisLibasyncAddWrite(void *privdata) {
    redisLibasyncEvents *e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selwrite, wrap(redisLibasyncWriteEvent, e));
}

void redisLibasyncDelWrite(void *privdata) {
    redisLibasyncEvents *e = (redisLibasyncEvents*)privdata;
    fdcb(e->context->c.fd, selwrite, nullptr);
}

void redisLibasyncCleanup(void *privdata) {
    redisLibasyncEvents *e = (redisLibasyncEvents*)privdata;
    redisLibasyncDelRead(privdata);
    redisLibasyncDelWrite(privdata);
    delete e;
}

int redisLibasyncAttach(redisAsyncContext *ac) {
    redisLibasyncEvents *e;

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

