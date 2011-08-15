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
  _read_stop = mkevent (_read_rv);
  _write_stop = mkevent (_write_rv);
}

//-----------------------------------------------------------------------

axprt_inner::~axprt_inner () 
{ 
  close (0);
  _read_rv.cancel ();
  _write_rv.cancel ();
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

tamed void
axprt_inner::send (ptr<const expr_t> x, evb_t ev)
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
  ev->trigger (!err);
}

//-----------------------------------------------------------------------

void axprt_inner::close (int en) 
{ 
  if (_fd >= 0) {
    close (_fd);
    _fd = -1; 
    _errno = en;
  }
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

str 
axprt_inner::get_remote ()
{
  if (!_remote && _fd >= 0) {
      socklen_t sinlen;
      struct sockaddr_in sin;
      if (getpeername (_fd, (struct sockaddr *)&sin, &sinlen) != 0) {
	warn ("getpeername failed on socket %d: %m\n", _fd);
      } else if (sinlen != sizeof (sockaddr_in)) {
	warn ("getpeername returned strange sockaddr, sized: %d\n", sinlen);
      } else {
	str ip = inet_ntoa (sin.sin_addr);
	int port = ntohs (sin.sin_port);
	_remote = strbuf ("%s:%d", ip.cstr (), port);
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
  warn << "axprt_inner(" << r << "): " << s << "\n";
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

    } else if (typ == RESPONSE) {

      callev_t::ptr *evp = _calls[seqid];
 
      if (!evp) {

      } else {
	callev_t::ptr ev = *evp;
	_calls.remove (seqid);
	callres_t res;
	if ((res.err_msg = (*l)[2])) {
	  res.set_err_code (RPC_CANTDECODERES);
	}
	if (l->size () == 4) { res.res = (*l)[3]; }
	ev->trigger (res);
      }
      
    } else {
      error (strbuf ("dispatch: msg type=%" PRId64 " is unknown", typ));
    }
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

  while (is_open () && !x) {

    if (first) { first = false; }
    else { twait { waitread (mkevent ()); } }
    
    if (!is_open ()) { /* noop */ }

    // EOF situation.
    else if ((rc = uio->input (_fd, ps)) == 0) {
      close (0);
      ret = 0;

    } else if (rc < 0 && errno == EAGAIN) {
      /* try again! */

    } else if (rc < 0) {
      ret = -1;
      close (errno);
      
    } else if (needed && uio->resid () < needed) {
      /* try again! */
      
    } else if ((x = msgpack::decode (get_str (needed), &mperrno, &len))) {

      uio->rembytes (len);
      ret = 1;

    } else if (mperrno != EAGAIN) {

      ret = -1;
      errno = mperrno;
      close (errno);
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

  // XXX todo -- make much more robust!
  // XXX todo -- handle notifies
  // XXX todo -- think about errors

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
    res.err_msg = expr_str_t::alloc (strbuf ("write error: %m"));
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
      close (errno);
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
  close (0);
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

ptr<axprt>
axprt::alloc (int fd)
{
  ptr<axprt> ret = New refcounted<axprt> (fd);
  ret->run ();
  return ret;
}

axprt::axprt (int fd) : _xi (axprt_inner::alloc (fd)) {} 
void axprt::run () { _xi->dispatch (); }
axprt::~axprt () { _xi->stop (); }

//=======================================================================

aclnt::~aclnt () {}

//-----------------------------------------------------------------------

tamed void
aclnt::call (str method, ptr<const expr_t> arg, callev_t::ptr ev)
{
  if (_x) {
    _x->call (method, arg, ev);
  } else if (ev) {
    callres_t res;
    res.set_err_code (RPC_CANTSEND);
    ev->trigger (res);
  }
}

//=======================================================================

}
}

