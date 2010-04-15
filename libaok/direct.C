#include "ok.h"
#include "parseopt.h"

//-----------------------------------------------------------------------

bool
ok_direct_ports_t::bind (const str &servpath, u_int32_t listenaddr)
{
  bool rc = true;

  for (size_t i = 0; i < _ports.size (); i++) {
    int port = _ports[i]._port;
    int fd = inetsocket (SOCK_STREAM, port, listenaddr);
    if (fd < 0) {
      warn ("Service (%s) cannot bind to port %d: %m\n",
	    servpath.cstr (), port);
      rc = false;
    } 
    _ports[i]._fd = fd;
  }
  return rc;
}

//-----------------------------------------------------------------------

void
ok_direct_ports_t::add_port_pair (const ok_portpair_t &p)
{
  _ports.push_back (p);
  _map.insert (p._port);
}

//-----------------------------------------------------------------------

void
ok_direct_ports_t::init (const vec<int> &v, size_t id, size_t n)
{
  _ports.clear (); // if reiniting after a crash

  //
  // MK 4/9/2010
  //
  // There are three allowable cases, for now.
  //
  // The simple & common case is no direct-bind ports.
  //
  // If there are multiple "brother" processes, then n > 1, and the
  // id is the ID of the current brother.  In this case, there must
  // be the same number of ports as there are brothers, each taking a 
  // port in sequence.
  //
  // If it's a single-child, then it's OK to have multiple port, that
  // single process binding to all ports.
  //
  // This is all a little bit of a hack, but the more general
  // alternative doesn't seem quite useful...
  //
  if (v.size () == 0) { /* then skip it all */ }
  else if (n == 1) {
    assert (id == 0);
    for (size_t i = 0; i < v.size (); i++) {
      add_port_pair (ok_portpair_t (v[i]));
    }
  } else {
    assert (n != 0);
    assert (n == v.size ());
    add_port_pair (ok_portpair_t (v[id]));
  }
}

//-----------------------------------------------------------------------

str
ok_direct_ports_t::encode_as_str () const
{
  strbuf b;
  for (size_t i = 0; i < _ports.size (); i++) {
    str s = _ports[i].encode_as_str ();
    if (i != 0) b << ",";
    b << s;
  }
  return b;
}

//-----------------------------------------------------------------------

str
ok_portpair_t::encode_as_str () const
{
  strbuf b;
  b << _port << ":" << _fd;
  return b;
}

//-----------------------------------------------------------------------

void
ok_portpair_t::close ()
{
  if (_fd >= 0) {
    ::close (_fd);
    _fd = -1;
  }
}

//-----------------------------------------------------------------------

void
ok_direct_ports_t::close ()
{
  for (size_t i = 0; i < _ports.size (); i++) { _ports[i].close (); }
}

//-----------------------------------------------------------------------

bool
ok_direct_ports_t::parse (const str &in)
{
  bool ret = true;
  vec<str> v;
  static rxx x (",");
  split (&v, x, in);

  for (size_t i = 0; i < v.size (); i++) {
    ok_portpair_t p;
    if (!p.parse (v[i])) {
      warn << "Cannot parse port pair: " << v[i] << "\n";
      ret = false;
    } else {
      add_port_pair (p);
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
ok_portpair_t::parse (const str &s)
{
  vec<str> v;
  static rxx x (":");
  split (&v, x, s);
  return (v.size () == 2 &&
	  convertint (v[0], &_port) &&
	  convertint (v[1], &_fd));
}

//-----------------------------------------------------------------------

void
ok_direct_ports_t::disable_accept ()
{
  for (size_t i = 0; i < _ports.size (); i++) {
    _ports[i].disable_accept ();
  }
}

//-----------------------------------------------------------------------

void
ok_direct_ports_t::enable_accept (ok_con_acceptor_t *s, int listenq)
{
  for (size_t i = 0; i < _ports.size (); i++) {
    _ports[i].enable_accept (s, listenq);
  }
}

//-----------------------------------------------------------------------

void
ok_portpair_t::enable_accept (ok_con_acceptor_t *s, int listenq)
{
  if (!_listening) {
    listen (_fd, listenq);
    _listening = true;
  } 
  fdcb (_fd, selread, 
	wrap (s, &ok_con_acceptor_t::accept_new_con, this));
}

//-----------------------------------------------------------------------

void
ok_portpair_t::disable_accept ()
{
  fdcb (_fd, selread, NULL);
}

//-----------------------------------------------------------------------

void
oksrvc_t::enable_direct_ports ()
{
  _direct_ports.enable_accept (this, ok_listen_queue_max);
}

//-----------------------------------------------------------------------

void
ok_direct_ports_t::report ()
{
  for (size_t i = 0; i < _ports.size (); i++) {
    _ports[i].report ();
  }
}

//-----------------------------------------------------------------------

void
ok_portpair_t::report ()
{
  warn << "++ enabled direct port " << _port << " on fd=" << _fd << "\n";
}

//-----------------------------------------------------------------------

void
oksrvc_t::disable_direct_ports ()
{
  _direct_ports.disable_accept ();
}

//-----------------------------------------------------------------------

void
oksrvc_t::accept_new_con (ok_portpair_t *p)
{
  socklen_t sinlen = sizeof (sockaddr_in);
  sockaddr_in *sin = (sockaddr_in *)xmalloc (sinlen);
  bzero (sin, sinlen);
  int nfd = accept (p->_fd, reinterpret_cast<sockaddr *> (sin), &sinlen);
  if (nfd >= 0) {
    ptr<ahttpcon> x = ahttpcon::alloc (nfd, sin, -1, -1, false, true);
    str n;
    ptr<demux_data_t> d = New refcounted<demux_data_t> (p->_port, false, n);
    ahttpcon_wrapper_t<ahttpcon> acw (x, d);
    newclnt (acw);
  }
}

//-----------------------------------------------------------------------
