// -*-c++-*-
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

#ifndef _LIBAHTTP_AHTTP
#define _LIBAHTTP_AHTTP

#include "arpc.h"
#include "suiolite.h"
#include "okconst.h"

#define AHTTP_MAXLINE 1024

//
// global objects for keeping track of nunmber of syscalls
//
extern syscall_stats_t *global_syscall_stats;
extern time_t global_ssd_last;
extern int n_ahttpcon;

//
// memory recycling code hacked in for now....
//
#define RECYCLE_LIMIT 2048
void recycle (suio *s);
void recycle (suiolite *s);
suiolite *suiolite_alloc (int mb, cbv::ptr cb);
suio *suio_alloc ();



class cbv_countdown_t : public virtual refcount {
public:
  cbv_countdown_t (cbv c) : cb (c) {}
  void reset (cbv c) { cb = c; }
  ~cbv_countdown_t () { (*cb) (); }
private:
  cbv cb;
};

//
// fdtosend is set up to be stack allocated, i presume, as an optimization;
// this should really be changed to be heap allocated, and to have some
// simple story for recyling. would be much cleaner, and less error
// prone.  this is future work.
//
struct fdtosend {
  const int fd;
  mutable bool closeit;
  fdtosend (int f, bool c, ptr<cbv_countdown_t> b = NULL)
    : fd (f), closeit (c), cb (b) {}
  ~fdtosend () { if (closeit) close (fd); }
  fdtosend (const fdtosend &f) : fd (f.fd), closeit (f.closeit), cb (f.cb)
  { f.closeit = false; }
  ptr<cbv_countdown_t> cb;
};

class ahttpcon_clone;
class ahttpcon : public virtual refcount 
{
  vec<u_int64_t> syncpts; // sync points

protected:
  vec<fdtosend> fdsendq;

public:
  ahttpcon (int f, sockaddr_in *s = NULL, int mb = -1,
	    int rcvlmt = -1, bool coe = true)
    : fd (f), rcbset (false), wcbset (false), bytes_recv (0), bytes_sent (0),
      eof (false), destroyed (false), out (suio_alloc ()), sin (s),
      recv_limit (rcvlmt < 0 ? int (ok_reqsize_limit) : rcvlmt),
      overflow_flag (false), ss (global_syscall_stats),
      sin_alloced (s != NULL)
  {
    //
    // bookkeeping for debugging purposes;
    //
    n_ahttpcon++;

    make_async (fd);
    if (coe) close_on_exec (fd);
    if (mb == -1) mb = SUIOLITE_DEF_BUFLEN;
    in = suiolite_alloc (mb, wrap (this, &ahttpcon::spacecb));
    set_remote_ip ();
  }
  bool ateof () const { return eof; }
  inline sockaddr_in *get_sin () const { return sin; }
  inline const str & get_remote_ip () const { return remote_ip; }
  virtual ~ahttpcon ();
  void sendfd (int sfd, bool closeit = true, ptr<cbv_countdown_t> cb = NULL);
  // void stopread ();
  void setrcb (cbi::ptr cb); // cb called when reading regular byte streams
  void seteofcb (cbv::ptr c) { eofcb = c; }
  void clone (ref<ahttpcon_clone> xc);
  void output ();
  void spacecb ();
  void error (int ec);
  void send (const strbuf &b, cbv::ptr cb);
  void sendv (const iovec *iov, int cnt, cbv::ptr cb = NULL);
  void copyv (const iovec *iov, int cnt);
  suiolite *uio () const { return in; }
  u_int get_bytes_sent () const { return bytes_sent; }

  void set_close_fd_cb (cbv cb) 
  { assert (!cbcd); cbcd = New refcounted<cbv_countdown_t> (cb); }

  void reset_close_fd_cb (cbv cb)
  {
    assert (cbcd); 
    cbcd->reset (cb);
  }

  ptr<cbv_countdown_t> get_close_fd_cb () { return cbcd; }

  static ptr<ahttpcon> alloc (int fd, sockaddr_in *s = NULL, 
			      int mb = SUIOLITE_DEF_BUFLEN)
  { return New refcounted<ahttpcon> (fd, s, mb); }
  bool closed () const { return fd < 0; }
  bool overflow () const { return overflow_flag; }
  int set_lowwat (int sz);

protected:
  void set_remote_ip ();
  virtual int dowritev (int cnt) { return out->output (fd, cnt); }
  virtual ssize_t doread (int fd);
  virtual void recvd_bytes (int n);
  inline void wrsync ();
  virtual void fail ();
  virtual void too_many_fds () { fail (); }
  virtual void fail2 () {} 
  void input ();
  bool enable_selread ();
  void disable_selread ();
  void call_drained_cb ();

  int fd;
  cbi::ptr rcb;
  cbv::ptr eofcb;
  cbv::ptr drained_cb;
  bool rcbset, wcbset;
  suiolite *in;
  int bytes_recv, bytes_sent;
  bool eof, destroyed;
  suio *out;
  sockaddr_in *sin;
  str remote_ip;
  int recv_limit;
  bool overflow_flag;
  syscall_stats_t *ss;
  struct sockaddr_in sin3;
  const bool sin_alloced;

  ptr<cbv_countdown_t> cbcd;
};

// for parent dispatcher, which will send fd's
class ahttpcon_dispatch : public ahttpcon
{
public:
  ahttpcon_dispatch (int f, int mb = SUIOLITE_DEF_BUFLEN) 
    : ahttpcon (f, NULL, mb) {}
  static ptr<ahttpcon_dispatch> alloc (int f, int mb = SUIOLITE_DEF_BUFLEN)
  { return New refcounted<ahttpcon_dispatch> (f, mb); }
protected:
  virtual int dowritev (int cnt);
};

// for child process listening for a file descriptor to come in
typedef callback<void, ptr<ahttpcon> >::ref listencb_t;
class ahttpcon_listen : public ahttpcon 
{
public:
  ahttpcon_listen (int f) : ahttpcon (f), fd_accept_enabled (false)
  { 
    // XXX - don't want to stop listening to okd! there should be no
    // channel limit here; setting recv_limit = 0 should achieve this.
    recv_limit = 0; 
  }
  void setlcb (listencb_t c);
  static ptr<ahttpcon_listen> alloc (int f) 
  { return New refcounted<ahttpcon_listen> (f); }
  void disable_fd_accept ();
  void enable_fd_accept ();
protected:
  virtual ssize_t doread (int fd);
  virtual void fail2 ();
  void too_many_fds () {}
  listencb_t::ptr lcb;
  bool fd_accept_enabled;
};

// for server process to peek () and then pass of the fd
typedef callback<void, str, int>::ref clonecb_t;
class ahttpcon_clone : public ahttpcon
{
public:
  ahttpcon_clone (int f, sockaddr_in *s = NULL, size_t ml = AHTTP_MAXLINE);
  ~ahttpcon_clone () ;
  void setccb (clonecb_t cb);
  int takefd ();

  static ptr<ahttpcon_clone> 
  alloc (int f, sockaddr_in *s = NULL, size_t ml = AHTTP_MAXLINE) 
  { return New refcounted<ahttpcon_clone> (f, s, ml); }

  void declone ();
  static u_int maxscan () { return 10 + OK_MAX_URI_LEN; }

protected:
  void recvd_bytes (int n);
  virtual void fail2 ();

private:
  void end_read ();
  str delimit (int n);
  void reset_delimit_state ();
  void trickle_cb (ptr<bool> destroyed_local);

  const size_t maxline;
  clonecb_t::ptr ccb;

  bool found;
  int delimit_state;
  int delimit_status;
  char *delimit_start;

  u_int bytes_scanned;
  bool decloned;
  int trickle_state;
  ptr<bool> destroyed_p;
  timecb_t *dcb;
};

ptr<ahttpcon> 
ahttpcon_aspawn (str execpath, cbv::ptr postforkcb, ptr<axprt_unix> *ctlx,
		 char *const *env);

int
ahttpcon_aspawn (str execpath, const vec<str> &arv, cbv::ptr pfcb,
		 int *ctlx, char *const *env);

int
ahttpcon_spawn (str execpath, const vec<str> &avs, 
		cbv::ptr postforkcb, bool async, char *const *env,
		int *ctlx);

extern int ahttpcon_spawn_pid;

bool http_server (listencb_t lcb, int port);


#endif /* _LIBAHTTP_AHTTP */
