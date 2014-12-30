// -*-c++-*-

#include "pub3msgpackrpc.h"
#include "pub3msgpack.h"
#include "tame_io.h"

namespace pub3 {

#if 0
}
#endif

namespace msgpack {

#if 0
}
#endif

//=======================================================================

callres_t::callres_t (clnt_stat s, str m)
  : err_code (s)
{
  set_err_msg (m);
}

//-----------------------------------------------------------------------

void
callres_t::set_err_msg (str m)
{
  if (!err_obj) { err_obj = expr_dict_t::alloc (); }
  err_obj->insert ("msg", m);
}

//=======================================================================

axprt_inner::axprt_inner (int f) : 
  _errno (0),
  _fd (f) , 
  _read_lock (tame::lock_t::OPEN),
  _write_lock (tame::lock_t::OPEN), 
  _read_rv (__FILE__, __LINE__),
  _write_rv (__FILE__, __LINE__)
{
  // A stop trigger to wrap all lowest-level reads
  // A trigger of false means that it stopped, a trigger of true
  // means that it waited successfully on i/o
  make_async (f);
  _read_stop = mkevent (_read_rv);
  _write_stop = mkevent (_write_rv);
}

//-----------------------------------------------------------------------

axprt_inner::~axprt_inner () 
{ 
  _read_rv.cancel ();
  _write_rv.cancel ();
}

//-----------------------------------------------------------------------------

void
axprt_inner::seteofcb(cbv::ptr cb) {
    _eofcb = cb;
}

//-----------------------------------------------------------------------

tamed void
axprt_inner::waitwrite (evv_t ev)
{
  if (is_open ()) {
    tame::waitwrite (_fd, mkevent (_write_rv));
    twait (_write_rv);
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
axprt_inner::waitread (evv_t ev)
{
  if (is_open ()) {
    tame::waitread (_fd, mkevent (_read_rv));
    twait (_read_rv);
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

void
axprt_inner::register_asrv (str prot, ptr<asrv> v)
{
  if (prot) { 
    _dispatch.insert (prot, v);
  } else {
    _dispatch_def = v;
  }
}

//-----------------------------------------------------------------------

tamed void
axprt_inner::send (ptr<const expr_t> x, evb_t::ptr ev)
{
  tvars {
    str dat;
    str len;
    size_t len_i;
    strbuf b;
    suio *uio;
    int rc (0);
    bool first (true);
    bool err (false);
    holdvar ptr<axprt_inner> hold (mkref (_self));
  }

  twait { _write_lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }
  dat = msgpack::encode (x);
  len_i = dat.len ();
  len = msgpack::encode (expr_int_t::alloc (len_i));

  b << len << dat;
  uio = b.tosuio ();
  
  while (is_open () && uio->resid () && !err) {
    if (first) { first = false; }
    else { twait { waitwrite (mkevent ()); } }
    if (is_open ()) {
      rc = uio->output (_fd);
      if (rc < 0 && errno != EAGAIN) { err = true; }
    }
  }

  _write_lock.release ();
  if (ev) ev->trigger (!err);
}

//-----------------------------------------------------------------------

void
axprt_inner::kill_servers ()
{
  qhash_iterator_t<str, ptr<asrv> > it (_dispatch);
  ptr<asrv> s;
  while (it.next (&s)) s->eof ();
  _dispatch.clear ();
  if (_dispatch_def) {
    _dispatch_def->eof ();
    _dispatch_def = NULL;
  }
}

//-----------------------------------------------------------------------

void
axprt_inner::kill_clients ()
{
  qhash_iterator_t<u_int32_t, callev_t::ptr> it (_calls);
  callev_t::ptr ev;
  callres_t res (RPC_CANTRECV, "EOF while waiting for reply");
  while (it.next (&ev)) ev->trigger (res);
  _calls.clear ();
}

//-----------------------------------------------------------------------

void 
axprt_inner::axprt_close (int en) 
{ 
  if (_fd >= 0) {
    close (_fd);
    _fd = -1; 
    _errno = en;

    if (_eofcb) {
        (*_eofcb) (); 
    }
  }

  kill_clients ();
  kill_servers ();
}

//-----------------------------------------------------------------------

str
axprt_inner::get_str (size_t s) const
{
  str ret;
  if (!s) ret = _inbuf;
  else {
    mstr m (s + 1);
    suio *uio = _inbuf.tosuio ();
    assert (s <= uio->resid ());
    uio->copyout (m.cstr(), s);
    m.setlen (s);
    ret = m;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
axprt_inner::set_remote (const sockaddr_in &sin)
{
  str ip = inet_ntoa (sin.sin_addr);
  int port = ntohs (sin.sin_port);
  _remote = strbuf ("%s:%d", ip.cstr (), port);
}

//-----------------------------------------------------------------------

str 
axprt_inner::get_remote ()
{
  if (!_remote && _fd >= 0) {
      socklen_t sinlen;
      struct sockaddr_in sin;
      if (getpeername (_fd, (struct sockaddr *)&sin, &sinlen) != 0) {
	error (strbuf ("getpeername failed on socket %d: %m", _fd));
      } else if (sinlen != sizeof (sockaddr_in)) {
	error (strbuf ("getpeername returned strange sockaddr, sized: %d", 
		       sinlen));
      } else {
	set_remote (sin);
      }
  }

  if (!_remote) { _remote = "<no-ip>"; }

  return _remote;
}

//-----------------------------------------------------------------------

void
axprt_inner::error (str s)
{
  str r = get_remote ();
  warn << "msgpack::axprt_inner(" << r << "): " << s << "\n";
}

//-----------------------------------------------------------------------

tamed void
axprt_inner::dispatch ()
{
  tvars {
    int rc;
    ptr<expr_t> x;
    ptr<expr_list_t> l;
    holdvar ptr<axprt_inner> hold (mkref (_self));
    bool ok (true);
    int64_t typ, seqid;
  }

  while (ok) {
    twait { recv (mkevent (rc, x)); }

    if (rc == 0) {
      ok = false;

    } else if (rc < 0) {
      error (strbuf ("dispatch: recv error: %m"));
      ok = false;

    } else if (!x) { 
      error ("dispatch: no incoming message");
      
    } else if (!(l = x->to_list ())) {
      error ("dispatch: incoming message wasn't a list");
      

    } else if (l->size () < 3 || l->size () > 4) {
      error (strbuf ("dispatch: incoming message unexpected size (%zu)",
		     l->size ()));

    } else if (!(*l)[0]->to_int (&typ)) {
      error ("dispatch: slot 0 wasn't an integer");

    } else if (!(*l)[1]->to_int (&seqid)) {
      error ("dispatch: slot 1 wasn't an integer");

    } else if (typ == REQUEST || typ == NOTIFY) {
      dispatch_call (l, typ, seqid);

    } else if (typ == RESPONSE) {
      dispatch_reply (l, seqid);
    } else {
      error (strbuf ("dispatch: msg type=%" PRId64 " is unknown", typ));
    }
  }
}

//-----------------------------------------------------------------------

static str 
split_prog (str in, str *mthd)
{
  static rxx x ("(.*)\\.([^.]+)");
  str ret;
  if (x.match (in)) {
    ret = x[1];
    *mthd = x[2];
  }
  return ret;
}

//-----------------------------------------------------------------------

void
axprt_inner::dispatch_call (ptr<expr_list_t> l, int64_t typ, int64_t seqid)
{
  str mthd, split_mthd;
  str prog;
  ptr<asrv> v, *vp (NULL);
  bool ok = false;
  clnt_stat stat (RPC_SUCCESS);

  if (!(mthd = (*l)[2]->to_str ()) || !mthd.len ()) {
    error ("dispatch_call: method not found");
    stat = RPC_CANTDECODEARGS;
  } else {
    if ((prog = split_prog (mthd, &split_mthd)) && 
	(vp = _dispatch[prog])) {
      v = *vp;
      mthd = split_mthd;
    }
    if (!v && !(v = _dispatch_def)) {
      error (strbuf ("dispatch_call: unknown method: %s", mthd.cstr ()));
      stat = RPC_PROCUNAVAIL;
    } else {
      ok = true;
      v->dispatch (seqid, mthd, (*l)[3]);
    }
  }

  if (!ok) {
    svccb b;
    b.rpc_err_obj ()("clnt_stat") = int (stat);
    send (b.to_json ());
  }

}

//-----------------------------------------------------------------------

void
axprt_inner::dispatch_reply (ptr<expr_list_t> l, int64_t seqid)
{
  callev_t::ptr *evp = _calls[seqid];
  
  if (evp) {
    callev_t::ptr ev = *evp;
    _calls.remove (seqid);
    callres_t res;
    ptr<expr_t> err = (*l)[2];
    ptr<expr_dict_t> d;
    if (err && !err->is_null ()) {
      int code = RPC_SYSTEMERROR;
      if ((d = err->to_dict ())) {
	pub3::obj_dict_t dobj (d);
	int64_t i;
	if (dobj("clnt_stat").to_int (&i)) {
	  code = i;
	} else if (dobj("accept_stat").to_int (&i)) {
	  switch (i) {
	  case PROC_UNAVAIL:
	    code = RPC_PROCUNAVAIL;
	    break;
	  default: break;
	  }
	} 
      }
      res.set_err_code (clnt_stat (code));
      if (d) {
	res.set_err_obj (d);
      }
    }
    if (l->size () == 4) { res.res = (*l)[3]; }
    ev->trigger (res);
  }
}



//-----------------------------------------------------------------------

tamed void
axprt_inner::recv_json (size_t needed, size_t ps, ev_t ev)
{
  tvars {
    int ret (0);
    ptr<expr_t> x;
    suio *uio;
    int rc;
    size_t len (0);
    int mperrno;
    bool first (true);
  }

  uio = _inbuf.tosuio ();

  if ( (uio->resid() >= needed) &&
       (x = msgpack::decode (get_str (needed), &mperrno, &len))) {
      uio->rembytes (len);
      ev->trigger(1, x);
      return;
  } 

  while (is_open () && !x) {

    if (first) { first = false; }
    else { twait { waitread (mkevent ()); } }
    
    if (!is_open ()) { /* noop */ }

    // EOF situation.
    else if ((rc = uio->input (_fd, ps)) == 0) {
      axprt_close (0);
      ret = 0;
    } else if (rc < 0 && errno == EAGAIN) {
        if (uio->resid()) {
            error(str("recv_json: EAGAIN with resid(): ") << uio->resid());
            axprt_close (0);
            ret = 0;
        }
    } else if (rc < 0) {
      ret = -1;
      axprt_close (errno);
    } else if (needed && uio->resid () < needed) {
      /* try again! */
    } else if ((x = msgpack::decode (get_str (needed), &mperrno, &len))) {
      uio->rembytes (len);
      ret = 1;
    } else if (mperrno != EAGAIN) {
      ret = -1;
      errno = mperrno;
      axprt_close (errno);
    } 
  }

  ev->trigger (ret, x);
}

//-----------------------------------------------------------------------

u_int32_t axprt_inner::seqid () { return _seqid++; }

//-----------------------------------------------------------------------

tamed void
axprt_inner::call (str mthd, ptr<const expr_t> arg, callev_t::ptr ev)
{
  tvars {
    pub3::obj_list_t l;
    bool ok;
    u_int32_t s;
    callres_t res;
  }

  s = seqid ();
  l[0] = ev ? REQUEST : NOTIFY;
  l[1] = s;
  l[2] = mthd;
  l[3] = arg->cast_hack_copy ();

  twait { send (l.obj (), mkevent (ok)); }

  if (!ev) { /* noop */ }
  else if (ok) {
    _calls.insert (s, ev);
  } else if (ev) {
    res.set_err_code (RPC_CANTSEND);
    res.set_err_msg (strbuf ("write error: %m"));
    ev->trigger (res);
  }
}

//-----------------------------------------------------------------------

tamed void
axprt_inner::recv (ev_t ev)
{
  tvars {
    int ret (0);
    ptr<expr_t> lx, x;
    int64_t len (0);
  }

  twait { _read_lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }

  twait { recv_json (0, 0x10, mkevent (ret, lx)); }
  if (ret > 0) {
      assert (lx);
      assert (is_open ());
      if (!lx->to_int (&len)) {
          errno = ENOENT;
          axprt_close (errno);
      } else {
          twait { recv_json (len, 0x2000, mkevent (ret, x)); }
      }
  }

  ev->trigger (ret, x);
  _read_lock.release ();
}

//-----------------------------------------------------------------------

ptr<axprt_inner> axprt_inner::alloc (int fd) 
{ return New refcounted<axprt_inner> (fd); };

//-----------------------------------------------------------------------

void
axprt_inner::stop ()
{
  axprt_close (0);
  evv_t::ptr e;
  if ((e = _read_stop)) {
    _read_stop = NULL;
    e->trigger ();
  }
  if ((e = _write_stop)) {
    _write_stop = NULL;
    e->trigger ();
  }
}

//=======================================================================

void axprt::call (str methd, ptr<const expr_t> arg, callev_t::ptr ev)
{ _xi->call (methd, arg, ev); }

//-----------------------------------------------------------------------

void axprt::register_asrv (str prog, ptr<asrv> v) 
{ _xi->register_asrv (prog, v);  }

//-----------------------------------------------------------------------

ptr<axprt>
axprt::alloc (int fd)
{
  ptr<axprt> ret = New refcounted<axprt> (fd);
  ret->run ();
  return ret;
}

//-----------------------------------------------------------------------------

void
axprt::seteofcb(cbv::ptr cb) {
    _xi->seteofcb(cb);
}

//-----------------------------------------------------------------------

axprt::axprt (int fd) : _xi (axprt_inner::alloc (fd)) {} 
void axprt::run () { _xi->dispatch (); }
axprt::~axprt () { _xi->stop (); }
void axprt::send (ptr<const expr_t> x, evb_t::ptr ev) { _xi->send (x, ev); }
str axprt::get_remote () const { return _xi->get_remote (); }
void axprt::set_remote (const sockaddr_in &s) { _xi->set_remote (s); } 

//=======================================================================

aclnt::~aclnt () {}

//-----------------------------------------------------------------------

aclnt::aclnt (ptr<axprt> x, str prog)
  : _x (x), _prog (prog) {}

//-----------------------------------------------------------------------

ptr<aclnt>
aclnt::alloc (ptr<axprt> x, str prog) 
{ return New refcounted<aclnt> (x, prog);  }

//-----------------------------------------------------------------------

str
aclnt::mkmthd (str s) const
{
  str ret;
  if (!_prog) { ret = s; }
  else { 
    strbuf b;
    b << _prog << "." << s;
    ret = b;
  }
  return ret;
}

//-----------------------------------------------------------------------

tamed void
aclnt::call (str method, ptr<const expr_t> arg, callev_t::ptr ev)
{
  if (_x) {
    _x->call (mkmthd (method), arg, ev);
  } else if (ev) {
    callres_t res;
    res.set_err_code (RPC_CANTSEND);
    ev->trigger (res);
  }
}

//=======================================================================

asrv::asrv (ptr<axprt> x, str prog, asrvcb_t cb)
  : _x (x),
    _prog (prog),
    _cb (cb)
{ _x->register_asrv (prog, mkref (this)); }

//-----------------------------------------------------------------------

ptr<asrv> asrv::alloc (ptr<axprt> x, str prog, asrvcb_t cb)
{ return New refcounted<asrv> (x, prog, cb); }

//-----------------------------------------------------------------------

void asrv::eof () { _cb (svccb ()); _x = NULL; }

//-----------------------------------------------------------------------

void
asrv::reply (const svccb &v)
{
  ptr<expr_t> x = v.to_json ();
  _x->send (x);
}

//-----------------------------------------------------------------------

void
asrv::error (str s)
{
  warn << "msgpack::asrv";
  if (_prog) {
    warnx << " (" << _prog << ")";
  }
  warnx << s << "\n";
}

//-----------------------------------------------------------------------

void
asrv::dispatch (seqid_t sid, str mthd, ptr<expr_t> arg)
{
  svccb v (sid, mthd, arg, mkref (this));
  _cb (v);
}

//=======================================================================

svccb::svccb (seqid_t s, str m, ptr<expr_t> a, ptr<asrv> v)
  : _sid (s),
    _mthd (m),
    _arg (a),
    _asrv (v),
    _eof (false) {}

//-----------------------------------------------------------------------

svccb::svccb () : _sid (0), _eof (true) {}

//-----------------------------------------------------------------------

pub3::obj_dict_t
svccb::rpc_err_obj ()
{
  pub3::obj_dict_t d;
  _err = d.obj ();
  return d;
}

//-----------------------------------------------------------------------

void
svccb::reject (accept_stat s)
{
  rpc_err_obj()("accept_stat") = s;
  send ();
}

//-----------------------------------------------------------------------

void
svccb::send ()
{
  ptr<axprt> x = _asrv->getx ();
  if (x) {
    x->send (to_json ());
  } else {
    _asrv->error ("svccb: cannot send");
  }
  _asrv = NULL;
}

//-----------------------------------------------------------------------

void svccb::reply (ptr<expr_t> x) 
{ 
  _res = x;
  send ();
}

//-----------------------------------------------------------------------

ptr<expr_t>
svccb::to_json () const
{
  pub3::obj_list_t o;
  o[0] = RESPONSE;
  o[1] = _sid;
  if (_err) { 
    o[2] = _err;
    o[3] = expr_null_t::alloc ();
  } else if (_res) {
    o[2] = expr_null_t::alloc ();
    o[3] = _res;
  }
  return o.obj ();
}

//=======================================================================

server_con_t::server_con_t (ptr<server_t> parent, ptr<axprt> x, str prog)
  : _parent (parent), 
    _x (x),
    _asrv (asrv::alloc (_x, prog, wrap (this, &server_con_t::handle_call))),
    _hold (mkref (this))
{}

//-----------------------------------------------------------------------

void server_con_t::release () { _hold = NULL; }

//-----------------------------------------------------------------------

void server_con_t::add_handler (str s, asrvcb_t cb)
{ _dispatch_tab.insert (s, cb); }

//-----------------------------------------------------------------------

void
server_con_t::handle_call (svccb b)
{
  if (b.eof ()) {
    handle_eof ();
  } else {
    asrvcb_t::ptr c = _dispatch_tab[b.method ()];
    if (!c) {
      b.reject (PROC_UNAVAIL);
    } else {
      (*c) (b);
    }
  }
}

//=======================================================================

server_t::server_t (u_int32_t port, str addr)
  : _port (port),
    _addr (addr),
    _rv (__FILE__, __LINE__),
    _fd (-1)
{
  _kill_ev = mkevent (_rv);
}

//-----------------------------------------------------------------------

void
server_t::kill ()
{
  if (_fd >= 0) {
    close (_fd);
    _fd = -1;
  }

  if (_kill_ev) {
    evv_t::ptr e = _kill_ev;
    _kill_ev = NULL;
    e->trigger ();
  }
  _rv.cancel ();
}

//-----------------------------------------------------------------------

server_t::~server_t () { kill (); }

//-----------------------------------------------------------------------

tamed void
server_t::accept_loop ()
{
  tvars {
    bool go (true);
  }
  while (go) {
    tame::waitread (_fd, mkevent (_rv));
    twait (_rv);
    if (_fd >= 0) {
      sockaddr_in sin;
      socklen_t sl = sizeof (sin);
      int fd = accept (_fd, reinterpret_cast<sockaddr *> (&sin), &sl);
      if (fd >= 0) {
	ref<axprt> x = axprt::alloc (fd);
	x->set_remote (sin);
	make_new_con (x);
      }
    } else {
      go = false;
    }
  }
}

//-----------------------------------------------------------------------

bool
server_t::bind ()
{
  bool ret = true;
  u_int32_t addr = INADDR_ANY;
  if (_addr) {
    struct in_addr ia;
    if (inet_aton (_addr.cstr (), &ia) <= 0) {
      warn ("Could not decode bind address %s: %m", _addr.cstr ());
      ret = false;
    } else {
      addr = ia.s_addr;
    }
  }

  if (ret && (_fd = inetsocket (SOCK_STREAM, _port, addr)) < 0) {
    str extra = "";
    if (_addr) { extra = strbuf ("%s:", _addr.cstr ()); }
    warn ("could not bind to port %s%d: %m", extra.cstr (), _port);
    ret = false;
  }

  if (ret) {
    msg (strbuf ("binding to port %d (fd=%d)", _port, _fd));
    make_async (_fd);
    close_on_exec (_fd);
    if (listen (_fd, 200) < 0) {
      warn ("could not call list on port: %m");
      ret = false;
    }
  }

  if (ret) {
    accept_loop ();
  }
  return ret;
}

//-----------------------------------------------------------------------

void
server_t::msg (str s) const
{
  if (_verbose) { warn << s << "\n"; }
}

//-----------------------------------------------------------------------------

tamed void
client_t::connect(evb_t ev) {
    
    tvars {
        bool ret (true);
        int fd;
    }

    twait { tcpconnect (_host, _port, mkevent (fd)); }
    if (fd < 0) {
        ret = false;
        if (!_retrying) {
            warn << "msgpack-rpc: cannot connect to host " << _host << ":" 
                << _port << "\n";
            conn_died();
        }
    } else {
        _x = pub3::msgpack::axprt::alloc (fd);
        _cli = pub3::msgpack::aclnt::alloc (_x, _prog);
        _x->seteofcb(wrap(this, &client_t::conn_died));
        _dead = false;
        warn << "msgpack-rpc: connection succeeded: host: " << _host
             << " port: " << _port << "\n";
    }

    ev->trigger (ret); 
}

//-----------------------------------------------------------------------------

tamed void
client_t::call(str method, ptr<pub3::expr_t> arg, callev_t::ptr ev) {

    tvars {
        pub3::msgpack::callres_t res;
    }

    if (_dead) {
        warn << "msgpack-rpc: call(): no connection available, failing...\n";
        res.err_code = RPC_CANTSEND;
    } else {
        twait { _cli->call(method, arg, mkevent(res)); }
    }

    if (ev) ev->trigger(res); 
}

//-----------------------------------------------------------------------------

void
client_t::conn_died() {
    warn << "msgpack-rpc: connection died: host: " << _host << " port: "
         << _port << "\n";
    _dead = true;
    if (_reconnect) {
        warn << "msgpack-rpc: attempting to reconnect...\n";
        retry_loop();
    }
}

//-----------------------------------------------------------------------------

tamed void
client_t::retry_loop() {
    
    tvars {
        bool ok(false);
        size_t attempts(0);
    }

    if (_retrying) return;

    _retrying = true;
    while (!ok) {
        twait { connect(mkevent(ok)); }
        if (ok) {
            warn << "msgpack-rpc: reconnected: host: " << _host 
                 << " port: " << _port << "\n";
        } else {
            warn << "msgpack-rpc: connection failed (attempts=" << attempts++
                 << "): host: " << _host 
                 << " port: " << _port << "\n";
            twait { delaycb(1, 0, mkevent()); }
        }
    }
    _retrying = false;
}


//=======================================================================

}
}

