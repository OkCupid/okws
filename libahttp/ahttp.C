/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "ahttp.h"
#include "abuf.h"
#include "httpconst.h"
#include <sys/types.h>
#include <sys/socket.h>
#include "okdbg.h"
#include "ahutil.h"

//
// hacked in here for now...
//
vec<suio *> recycled_suios;
vec<suiolite *> recycled_suiolites;
vec<suiolite *> recycled_suiolites_small;

syscall_stats_t *global_syscall_stats = NULL;
time_t global_ssd_last = 0;
int n_ahttpcon = 0;


ahttpcon::ahttpcon (int f, sockaddr_in *s, int mb, int rcvlmt, bool coe,
                    bool ma)
  : start (sfs_get_timenow ()), fd (f), rcbset (false),
    wcbset (false), _bytes_recv (0), bytes_sent (0),
    eof (false), destroyed (false), out (suio_alloc ()), sin (s),
    recv_limit (rcvlmt < 0 ? int (ok_reqsize_limit) : rcvlmt),
    overflow_flag (false), ss (global_syscall_stats),
    sin_alloced (s != NULL),
    _timed_out (false),
    _no_more_read (false),
    _delayed_close (false),
    _zombie_tcb (NULL),
    _state (AHTTPCON_STATE_NONE),
    destroyed_p (New refcounted<bool> (false)),
    _remote_port (0),
    _source_hash (0),
    _source_hash_ip_only (0),
    _reqno (0)
{
  //
  // bookkeeping for debugging purposes;
  //
  n_ahttpcon++;

  if (ma) make_async (fd);
  if (coe) close_on_exec (fd);
  if (mb < 0) mb = SUIOLITE_DEF_BUFLEN;
  in = suiolite_alloc (mb, wrap (this, &ahttpcon::spacecb));
  set_remote_ip ();

  if (ok_ahttpcon_zombie_warn && ok_ahttpcon_zombie_timeout > 0) {
    _zombie_tcb = delaycb (ok_ahttpcon_zombie_timeout, 0,
			   wrap (this, &ahttpcon::zombie_warn, destroyed_p));
  }

}

int
ahttpcon::takefd ()
{
  int ret = fd;
  if (fd >= 0) {
    wcbset = false;
    rcbset = false;
    fdcb (fd, selread, NULL);
    fdcb (fd, selwrite, NULL);
    fd = -1;
  }
  return ret;
}

int
ahttpcon_clone::takefd ()
{
  int ret = ahttpcon::takefd ();
  ccb = NULL;
  return ret;
}

void
ahttpcon::send (const strbuf &b, cbv::ptr drained, cbv::ptr sent)
{
  return sendv (b.iov (), b.iovcnt (), drained, sent);
}

void
ahttpcon::copyv (const iovec *iov, int cnt)
{
  out->copyv (iov, cnt, 0);
}

void
ahttpcon::sendv (const iovec *iov, int cnt, cbv::ptr drained,
		 cbv::ptr sent)
{
  _state = AHTTPCON_STATE_SEND;
  assert (!destroyed);
  u_int32_t len = iovsize (iov, cnt);
  if (fd < 0) {
    // if an EOF happened after the read but before the send,
    // we'll wind up here.
    warn ("write not possible due to EOF\n");
    if (sent) (*sent) ();
    if (drained) (*drained) ();
    return;
  }
  bytes_sent += len;
  if (!out->resid () && cnt < min (16, UIO_MAXIOV)) {
    if (ss) ss->n_writev ++;
    ssize_t skip = writev (fd, iov, cnt);
    if (skip < 0 && errno != EAGAIN) {
      fail ();
      // still need to signal done sending....
      if (sent) (*sent) ();
      if (drained) (*drained) ();
      return;
    } else
      out->copyv (iov, cnt, max<ssize_t> (skip, 0));
  } else {
    out->copyv (iov, cnt, 0);
  }
  drained_cb = drained;
  if (sent)
    out->iovcb (sent);
  output (destroyed_p);
}

void
ahttpcon::call_drained_cb ()
{
  cbv::ptr c = drained_cb;
  if (c) {
    drained_cb = NULL;
    (*c) ();
  }
}

void
ahttpcon::set_drained_cb (cbv::ptr cb)
{
  if (!cb) {
    drained_cb = NULL;
  } else if (!out->resid () || fd < 0) {
    (*cb) ();
  } else {
    // hold onto the current drained_cb until after we access
    // this (by setting a new drained_cb)
    cbv::ptr c = drained_cb;
    drained_cb = cb;
  }
}

void
ahttpcon::output (ptr<bool> destroyed_local)
{
  if (*destroyed_local)
    return;
  if (fd < 0)
    return;

  ssize_t n = 0;
  int cnt = 0;
  do {
    cnt = -1 ;
  } while ((n = dowritev (cnt)) > 0);

  bool more_to_write = out->resid ();
  if (n < 0) {
    fail ();
  } else if (more_to_write && !wcbset) {
    wcbset = true;
    fdcb (fd, selwrite, wrap (this, &ahttpcon::output, destroyed_p));
  }
  else if (!more_to_write && wcbset) {
    wcbset = false;
    fdcb (fd, selwrite, NULL);
  }

  if (!out->resid () && drained_cb) {
    call_drained_cb ();
  }
}

void
ahttpcon::setrcb (cbi::ptr cb)
{
  if (!cb) {
    rcb = NULL;
    return;
  }

  _state = AHTTPCON_STATE_RECV;

  if (enable_selread ()) {
    rcb = cb;
    int i = in->resid ();
    if (i)
      (*rcb) (i);
  } else {
    (*cb) (0);
  }
}

ahttpcon_clone::ahttpcon_clone (int f, bool all_headers, sockaddr_in *s,
                                size_t ml)
  : ahttpcon (f, s, ml), _all_headers(all_headers), maxline (ml), ccb (NULL),
    decloned (false)
{ }

//-----------------------------------------------------------------------

void
ahttpcon::zombie_warn (ptr<bool> df)
{
  static const char *prfx = "XX AHTTPCON_ZOMBIE: ";
  if (*df) {
    warn << prfx << "warning fired on deleted object!\n";
  } else {
    str ip = get_remote_ip ();
    if (!ip) ip = "<none>";
    warn ("%sfd=%d, state=%d, timeout=%d, ip=%s\n",
	  prfx, fd, int (_state), int (ok_ahttpcon_zombie_timeout), ip.cstr ());
    _zombie_tcb = NULL;
  }
}

//-----------------------------------------------------------------------

ahttpcon::~ahttpcon ()
{
  //
  // bookeeping for debug purposes.
  //
  --n_ahttpcon;
  ahttpcon_byte_counter.update (get_bytes_sent (), get_bytes_recv ());

  destroyed = true;
  *destroyed_p = true;
  fail ();
  if (sin && sin_alloced) xfree (sin);
  recycle (in);
  recycle (out);
  if (_zombie_tcb) {
    timecb_remove (_zombie_tcb);
    _zombie_tcb = NULL;
  }
}

void
ahttpcon::short_circuit_output ()
{
  _delayed_close = true;
}

/*
 * In some cases, it helps to delay the close so that the bytes sent
 * to the client can go out.  In particular, this is useful (in some
 * tests) when the server tries to reply before it has read the
 * entire incoming request.  Leave the code in, but it hasn't helped
 * with FF or IE.  It does help against netcat.
 */
static void
void_close (int fd)
{
  (void)close (fd);
}

void
ahttpcon::fail ()
{
  if (fd >= 0) {
    rcbset = false;
    wcbset = false;
    fdcb (fd, selread, NULL);
    fdcb (fd, selwrite, NULL);
    if (_delayed_close) {
      delaycb (1, 0, wrap (void_close, fd));
    } else {
      close (fd);
    }
  }
  fd = -1;
  if (!destroyed) {
    ref<ahttpcon> hold (mkref (this));
    if (rcb)
      (*rcb) (0);
    else if (eofcb)
      (*eofcb) ();
    else if (drained_cb) {
      call_drained_cb ();
    }
    fail2 ();
  }
}

void
ahttpcon_clone::fail2 ()
{
  issue_ccb (_reqno > 0 ? HTTP_PIPELINE_EOF : HTTP_TIMEOUT);
}

void
ahttpcon_clone::setccb (clonecb_t c, size_t num_preload_bytes)
{
  assert (!destroyed);

  _state = AHTTPCON_STATE_DEMUX;
  ccb = c;

  if (num_preload_bytes) {
    _bytes_recv += num_preload_bytes;
    recvd_bytes (num_preload_bytes);
  } else if (!enable_selread ()) {
    read_fail (HTTP_BAD_REQUEST);
  }
}

void
ahttpcon::spacecb ()
{
  enable_selread ();
}

bool
ahttpcon::enable_selread ()
{
  if (fd < 0 || _no_more_read)
    return false;
  if (!rcbset) {
    rcbset = true;
    fdcb (fd, selread, wrap (this, &ahttpcon::input, destroyed_p));
  }
  return true;
}

void
ahttpcon::stop_read ()
{
  /*
   * half-way close code; does not achieve the purpose intended,
   * which is to reply to an HTTP req before reading in the whole
   * req.
   */
  if (!_no_more_read && fd >= 0)  {
    _no_more_read = true;
    disable_selread ();

    shutdown (fd, SHUT_RD);
    warn << "trying 1-way shutdown!\n";

    // read all packets from the kernel buffer
#define BUFSZ 65000
    char buf[BUFSZ];
    ssize_t rc;
    while ((rc = read (fd, buf, BUFSZ)) > 0) {}
#undef BUFSZ

  }
}

void
ahttpcon::disable_selread ()
{
  rcbset = false;
  fdcb (fd, selread, NULL);
}

ssize_t
ahttpcon::doread (int fd)
{
  ssize_t s = in->input (fd, NULL, ss);
  return s;
}

void
ahttpcon::input (ptr<bool> destroyed_local)
{
  if (*destroyed_local)
    return;
  if (fd < 0)
    return;
  ref<ahttpcon> hold (mkref (this));  // Don't let this be freed under us
  if (in->full ()) {
    rcbset = false;
    fdcb (fd, selread, NULL);
    return;
  }
  ssize_t n = doread (fd);
  if (n < 0) {

    if (errno == EMSGSIZE) {
      warn ("Too many fds (%d) in ahttpcon::input: %m\n", n_ahttpcon);
      too_many_fds ();
    } else if (errno == ECONNRESET) {
      eof = true;
      fail ();
    } else if (errno != EAGAIN) {
      warn ("nfds=%d; Error in ahttpcon::input (%s): %m (%d)\n",
	    n_ahttpcon, get_remote_ip ().cstr (), errno);
      fail ();
    }
    return;
  }

  //
  // MK 12/27/07: Back-out earlier change here, in which for n==0, still
  // call recvd_bytes(0), to go through the normal processing path.  That
  // might be the right thing to do if there are browsers that do
  // TCP half-closes.  I don't think there are, so just leave it as
  // is so I don't break anything.
  //
  if (n == 0) {
    eof = true;
    fail ();
    return;
  }

  _bytes_recv += n;
  // stop DOS attacks?
  if (recv_limit > 0 && _bytes_recv > recv_limit) {
    warn << "Channel limit exceeded ";
    if (remote_ip)
      warnx << "(" << remote_ip << ")";
    warnx << " recv limit: " << recv_limit << ", bytes received: "
          << _bytes_recv << "\n";
    eof = true;
    disable_selread ();
    overflow_flag = true;
    n = 0;
  }

  recvd_bytes (n);
}

void
ahttpcon::recvd_bytes (size_t n)
{
  if (rcb)
    (*rcb) (n);
}

void
ahttpcon_clone::recvd_bytes (size_t n)
{
  if (decloned) {
    ahttpcon::recvd_bytes (n);
    return;
  }

  while (in->resid()) {
      ssize_t nbytes;
      char *p = in->getdata(&nbytes);
      request_bytes.setsize(request_bytes.size() + nbytes);
      memcpy(&request_bytes[request_bytes.size() - nbytes], p, nbytes);
      in->rembytes(nbytes);
  }

  int delimit_status = HTTP_OK;
  ptr<ahttp_delimit_res> dres;

  if (_all_headers) {
      dres = delimit_headers(&delimit_status);
  } else {
      dres = delimit(&delimit_status);
  }

  if (dres || delimit_status != HTTP_OK) {

    if (ccb) {
      (*ccb) (dres, delimit_status);
      ccb = NULL;
    }

    // Tricky! Need to check decloned flag again, because it might
    // have been set from within (*ccb). If the header is more than
    // one packet, then we should not be turning off reads, since the
    // header needs to be parsed for logging purposes
    if (dres && !decloned)
      end_read ();
    else {
      //
      // if it was an error, we still need to parse the rest of the
      // headers, and for that, we'll need to keep reading on this
      // ahttpcon_clone connection
      //
      // we should only parse the header here, and therefore should
      // read much less data
      recv_limit = ok_hdrsize_limit;
    }
  }
}

void
ahttpcon_clone::end_read ()
{
  if (fd >= 0) {
    rcbset = false;
    fdcb (fd, selread, NULL);
  }
}

void
ahttpcon_clone::declone ()
{
  decloned = true;
}

str
ahttp_delimit_res::get_forwarded_ip_str() const {
    str proxip = get_proxied_ip(m_headers);
    return proxip;
}

uint32_t
ahttp_delimit_res::get_forwarded_ip() const {
    str ipstr = get_forwarded_ip_str();
    if (ipstr) {
        return inet_addr(ipstr.cstr());
    }
    return 0;
}

ptr<ahttp_delimit_res>
ahttpcon_clone::delimit_headers(int* delimit_status) {

    int delimit_state = 0;

    str reqline;
    const char* nextline = nullptr;
    bool done = false;
    vec<str> headers;

    for (const char& p: request_bytes) {
        switch (delimit_state) {
        case 0:
            // Getting a newline char here means this is a blank line, kick out
            if (p == '\r' || p == '\n') {
                done = true;
                break;
            }
            nextline = &p;
            delimit_state = 1;
        case 1:
            // May be an end of a line
            if (p == '\r' || p == '\n')
                delimit_state = 2;
        case 2:
            if (p == '\n') {
                str header = str(nextline, &p - nextline);
                // tack on crlf to make sure parse_reqline() works
                header = header << "\r\n";

                // This is the reqline if we haven't gotten one yet
                if (!reqline) {
                    reqline = header;
                }

                // If this is a blank line, bail
                if (header == "\r\n") {
                    done = true;
                } else {
                    headers.push_back(tolower_s(header));
                }

                delimit_state = 0;
            } else {
                delimit_state = 1;
            }
            break;
        };

        if (done) break;
    }

    // If we don't have a reqline yet, no point in continuing
    if (!reqline) return nullptr;

    // Try to parse the reqline, if it comes back with an error then bail out
    str reqtarget = parse_reqline(reqline.cstr(), reqline.len(), delimit_status);
    if (!reqtarget || *delimit_status != HTTP_OK)
        return nullptr;

    // Don't fully proceed unless we get all the headers
    if (!done) return nullptr;

    ptr<ahttp_delimit_res> res =
        New refcounted<ahttp_delimit_res>(reqtarget);

    // Parse the headers
    for (str header: headers) {
        const char* chead = header.cstr();
        const char* caddr = strchr(chead, ':');
        if (!caddr) continue;
        size_t cpos = (size_t)(caddr-chead+1);

        str key = str(chead, cpos-1);
        str value = strip_newlines(str(chead+cpos+1, header.len()-cpos-1));
        res->add_header(key, value);
    }

    return res;
}

ptr<ahttp_delimit_res>
ahttpcon_clone::delimit (int *delimit_status) {
    str reqline = parse_reqline(request_bytes, request_bytes.size(), delimit_status);
    return New refcounted<ahttp_delimit_res>(reqline);
}

int
ahttpcon::set_lowwat (int lev)
{
  return setsockopt (fd, SOL_SOCKET, SO_RCVLOWAT, (char *)&lev, sizeof (lev));
}

void
ahttpcon::set_remote_ip ()
{
  if (!sin && !ok_send_sin) {
    socklen_t sinlen;
    if (getpeername (fd, (struct sockaddr *)&sin3, &sinlen) != 0) {
      warn ("getpeername failed on socket %d: %m\n", fd);
    } else if (sinlen != sizeof (sockaddr_in)) {
      warn ("getpeername returned strange sockaddr, sized: %d\n", sinlen);
    } else {
      sin = &sin3;
    }
  }

  if (sin) {
    remote_ip = inet_ntoa (sin->sin_addr);
    _remote_port = ntohs (sin->sin_port);
  } else {
    remote_ip = "0.0.0.0";
    _remote_port = 0;
  }
}

hash_t
ahttpcon::source_hash () const
{
  if (_source_hash == 0) {
    strbuf b ("%s:%d", remote_ip.cstr (), _remote_port);
    str s = b;
    _source_hash = s;
  }
  return _source_hash;
}

hash_t
ahttpcon::source_hash_ip_only () const
{
  if (_source_hash_ip_only == 0) {
    _source_hash_ip_only = remote_ip;
  }
  return _source_hash_ip_only;
}

void
recycle (suio *s)
{
  if (ok_recycle_suio_limit &&
      recycled_suios.size () < ok_recycle_suio_limit) {
    s->clear ();
    recycled_suios.push_back (s);
  } else {
    delete s;
  }
}

void
recycle (suiolite *s)
{
  if (ok_recycle_suio_limit &&
      s->getlen () == AHTTP_MAXLINE &&
      recycled_suiolites_small.size () < ok_recycle_suio_limit) {
    s->clear ();
    recycled_suiolites_small.push_back (s);
  } else if (ok_recycle_suio_limit &&
	     s->getlen () == SUIOLITE_DEF_BUFLEN &&
	     recycled_suiolites.size () < ok_recycle_suio_limit) {
    s->clear ();
    recycled_suiolites.push_back (s);
  } else {
    delete s;
  }
}

suio *
suio_alloc ()
{
  if (recycled_suios.size ()) {
    return recycled_suios.pop_front ();
  } else {
    return New suio ();
  }
}

suiolite *
suiolite_alloc (int mb, cbv::ptr s)
{
  suiolite *ret;
  if (recycled_suiolites.size () && mb == SUIOLITE_DEF_BUFLEN) {
    ret = recycled_suiolites.pop_front ();
    ret->recycle (s);
  } else if (recycled_suiolites_small.size () && mb == AHTTP_MAXLINE) {
    ret = recycled_suiolites_small.pop_front ();
    ret->recycle (s);
  } else {
    ret = New suiolite (mb, s);
  }
  return ret;
}

void
ahttp_tab_t::reg (ahttpcon *a, ptr<bool> d)
{
  ahttp_tab_node_t *n = New ahttp_tab_node_t (a, d);
  q.insert_tail (n);
  nent++;
}

void
ahttp_tab_t::unreg (ahttp_tab_node_t *n)
{
  q.remove (n);
  delete n;
  nent--;
}

void
ahttp_tab_t::sched ()
{
  if (!_shutdown)
    dcb = delaycb (interval, 0, wrap (this, &ahttp_tab_t::run));
}

str
ahttpcon::select_set () const
{
  strbuf b;
  if (rcbset) b << "r";
  if (wcbset) b << "w";
  return b;
}

//-----------------------------------------------------------------------

abuf_src_t *
ahttpcon::alloc_abuf_src ()
{
  return New abuf_con_t (mkref (this));
}

//-----------------------------------------------------------------------

static void true_cb (evb_t ev) { ev->trigger (true); }

void
ahttpcon::drain_to_network (ptr<strbuf> b, evb_t ev)
{
  send (*b, wrap (true_cb, ev));
}

//-----------------------------------------------------------------------

void
ahttpcon::drain_cancel ()
{
  set_drained_cb (NULL);
  cancel ();
}

//-----------------------------------------------------------------------

byte_counter_t ahttpcon_byte_counter;

//-----------------------------------------------------------------------

void
byte_counter_t::update (ssize_t s, ssize_t r)
{
  _n_sent += s;
  _n_recv += r;
}

//-----------------------------------------------------------------------

void
byte_counter_t::clear ()
{
  _n_sent = 0;
  _n_recv = 0;
}

//-----------------------------------------------------------------------

void
byte_counter_t::query (size_t *s, size_t *r)
{
  *s = _n_sent;
  *r = _n_recv;
}

//-----------------------------------------------------------------------

void
byte_counter_t::query_and_clear (size_t *s, size_t *r)
{
  query (s, r);
  clear ();
}

//-----------------------------------------------------------------------

void
ahttp_tab_t::kill_all ()
{
  ahttp_tab_node_t *n;
  while (( n = q.first) ) {
    if (! *n->_destroyed_p ) {
      ptr<ahttpcon> a = mkref (n->_a);
      str ai = a->all_info ();
      warn << "HTTP connection killed due to shutdown " << ai << "\n";
      a->kill ();
    }
    unreg (n);
  }
}

//-----------------------------------------------------------------------

void
ahttpcon::hit_timeout ()
{
  _timed_out = true;
  read_fail (HTTP_TIMEOUT);
}

//-----------------------------------------------------------------------

void
ahttpcon::kill ()
{
  // Just close the connection, don't send an HTTP response
  read_fail (HTTP_PIPELINE_EOF);
}

//-----------------------------------------------------------------------

str
ahttpcon::all_info () const
{
  str ss = select_set();
  str dbi = get_debug_info ();
  str ip = get_remote_ip ();
  if (!dbi) dbi = "";
  if (!ss) ss = "";
  strbuf b ("(ip=%s; fd=%d; bytes=%d; select=%s%s)",
	      ip.cstr (), getfd (), bytes_recv (),
	      ss.cstr (), dbi.cstr ());
  return b;

}

//-----------------------------------------------------------------------

void
ahttp_tab_t::run ()
{
    dcb = NULL;
    ahttp_tab_node_t *n;

    assert (!_shutdown);

    bool flag = true;
    while ((n = q.first) && flag) {
        if (* n->_destroyed_p ) {
            unreg (n);
        } else {
            ptr<ahttpcon> a = mkref (n->_a); // hold onto this

            // MM: If a keep-alive conn has timed out, just kill it here hard
            if (a->get_reqno() > 0 && !a->bytes_recv() &&
                sfs_get_timenow() - n->_a->start > ok_ka_timeout) {
                a->cancel();
                unreg(n);
                continue;
            }

            // MM: handle non-keep alive and normal connections with the
            // demux timeout procedure
            if ((a->get_reqno() == 0 || a->bytes_recv() > 0) &&
                int (sfs_get_timenow() - n->_a->start) >
                int (ok_demux_timeout)) {
                str ai = a->all_info ();
                warn << "HTTP connection timed out in demux " << ai << "\n";
                a->hit_timeout ();
                unreg (n);
            }  else {
                flag = false;
            }
        }
    }
    sched ();
}

//-----------------------------------------------------------------------

void
ahttp_tab_t::shutdown ()
{
  if (dcb) {
    timecb_remove (dcb);
    dcb = NULL;
  }
  _shutdown = true;
  kill_all ();
}

//-----------------------------------------------------------------------

void
ahttpcon_clone::issue_ccb (int s)
{
  if (ccb) {
    clonecb_t::ptr c = ccb;
    ccb = NULL;
    (*c) (NULL, s);
  }
}

//-----------------------------------------------------------------------

void
ahttpcon_clone::read_fail (int s)
{
  if (fd >= 0) {
    rcbset = false;
    fdcb (fd, selread, NULL);
    _no_more_read = true;
  }
  issue_ccb (s);
}

//-----------------------------------------------------------------------

size_t
ahttpcon::set_keepalive_data (const keepalive_data_t &kad)
{
  _reqno = kad._reqno;
  size_t ret = 0;


  OKDBG4(OKD_KEEPALIVE, CHATTER, "set_keepalive_data(reqno=%d, len=%zu, fd=%d)",
	 kad._reqno, kad._len, fd);

  if ((ret = kad._len) && kad._buf) {
    in->load_from_buffer (kad._buf, ret);
  }
  return ret;
}

//-----------------------------------------------------------------------

cidr_mask_t::cidr_mask_t(const char* range)
{

    m_desc = str(range);

    rxx pat("([^/]+)(/([0-9]+))?");
    net = 0xFFFFFFFF;
    mask = 0xFFFFFFFF;

    if (pat.match(range)) {
        str saddr = pat[1];
        struct in_addr addr;
        if (inet_aton(saddr.cstr(), &addr)) {
            net = ntohl(addr.s_addr);
            if (pat[3]) {
                uint32_t shift = 0;
                pat[3].to_uint32(&shift);
                mask = mask << (32 - shift);
                net &= mask;
            }
        }
        else {
            warn << "OK_ERROR: Invalid address: " << saddr << "\n";
        }
    }
    else {
        warn << "OK_ERROR: Invalid CIDR block: " << range << "\n";
    }
}

//-----------------------------------------------------------------------

bool cidr_mask_t::match(uint32_t ip) const
{
    return ((ip & mask) == net);
}

//-----------------------------------------------------------------------------

void
cidr_filter_t::add_mask(const cidr_mask_t& mask) {
    m_masks.push_back(mask);
}

//-----------------------------------------------------------------------------

bool
cidr_filter_t::match(uint32_t ip) const {
    for (const auto& m : m_masks) {
        if (m.match(ip)) return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

str
cidr_filter_t::encode() const {
    strbuf encstr;
    for (size_t i = 0; i < m_masks.size(); i++) {
        if (i != 0) encstr << ",";
        encstr << m_masks[i].desc();
    }
    return encstr;
}

//-----------------------------------------------------------------------------

void
cidr_filter_t::decode(str s) {
    vec<str> toks;
    static rxx x(",");
    split(&toks, x, s);

    m_masks.clear();
    for (auto t : toks) {
        m_masks.emplace_back(t.cstr());
    }
}

//-----------------------------------------------------------------------

