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

axprt::axprt (int f) : 
  _errno (0),
  _fd (f) , 
  _write_lock (tame::lock_t::OPEN), 
  _read_lock (tame::lock_t::OPEN) {}

//-----------------------------------------------------------------------

axprt::~axprt () { close (0); }

//-----------------------------------------------------------------------

tamed void
axprt::send (ptr<const expr_t> x, evi_t ev)
{
  tvars {
    int ret (0);
    str dat;
    str len;
    size_t len_i;
    strbuf b;
    suio *uio;
    int rc (0);
    bool first (true);
    bool err (false);
  }

  twait { _write_lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }
  dat = msgpack::encode (x);
  len_i = dat.len ();
  len = msgpack::encode (expr_int_t::alloc (len_i));

  b << len << dat;
  uio = b.tosuio ();
  
  while (uio->resid () && !err) {
    if (first) { first = false; }
    else { twait { tame::waitwrite (_fd, mkevent ()); } }
    rc = uio->output (_fd);
    if (rc < 0 && errno != EAGAIN) { err = true; }
  }
  if (err) { rc = -1; }

  _write_lock.release ();
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

void axprt::close (int en) { 
  if (_fd >= 0) {
    close (_fd);
    _fd = -1; 
    _errno = en;
  }
}

//-----------------------------------------------------------------------

str
axprt::get_str (size_t s) const
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

tamed void
axprt::recv_json (size_t needed, size_t ps, ev_t ev)
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
    else { twait { tame::waitread (_fd, mkevent ()); } }

    rc = uio->input (_fd, ps);

    // EOF situation.
    if (rc == 0) {
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

tamed void
axprt::recv (ev_t ev)
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

ptr<axprt> axprt::alloc (int fd) { return New refcounted<axprt> (fd); };

//=======================================================================

//=======================================================================
//=======================================================================

}
}

