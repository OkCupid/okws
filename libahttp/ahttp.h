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
#include "httpconst.h"

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

class ahttpcon_clone;
class ahttpcon : public virtual refcount 
{

public:
  /**
   * Analogous to axprt in SFS, this is a wrapper around a remote
   * HTTP connection object.
   *
   * @brief allocate a new HTTP connection wrapper object
   * @param f the file descriptor of the socket
   * @param s the sockaddr that came with accept()
   * @param mb input buffer size (-1 means use the default)
   * @param rcvlmt maximum bytes we can read before giving up (-1 for default)
   * @param coe call close_on_exec() on f
   * @param ma call make_async() on f
   */
  ahttpcon (int f, sockaddr_in *s = NULL, int mb = -1,
	    int rcvlmt = -1, bool coe = true, bool ma = true) ;

  int getfd () const { return fd; }
  bool ateof () const { return eof; }
  inline sockaddr_in *get_sin () const { return sin; }
  inline const str & get_remote_ip () const { return remote_ip; }
  virtual ~ahttpcon ();
  void setrcb (cbi::ptr cb); // cb called when reading regular byte streams
  void seteofcb (cbv::ptr c) { eofcb = c; }
  void output ();
  void spacecb ();
  void error (int ec);
  void send (const strbuf &b, cbv::ptr drained, cbv::ptr sent = NULL);
  void sendv (const iovec *iov, int cnt, cbv::ptr drained = NULL,
	      cbv::ptr sent = NULL);
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
			      int mb = -1, int rcvlimit = -1, 
			      bool coe = true, bool ma = true)
  { return New refcounted<ahttpcon> (fd, s, mb, rcvlimit, coe, ma); }
  bool closed () const { return fd < 0; }
  bool overflow () const { return overflow_flag; }
  int set_lowwat (int sz);
  bool timed_out () const { return _timed_out; }
  void set_drained_cb (cbv::ptr cb);
  void cancel () { fail (HTTP_CLIENT_EOF); }
  void stop_read ();
  void short_circuit_output ();
  int bytes_recv () const { return _bytes_recv;}
  
  const time_t start;

protected:
  void set_remote_ip ();
  virtual int dowritev (int cnt) { return out->output (fd, cnt); }
  virtual ssize_t doread (int fd);
  virtual void recvd_bytes (int n);
  virtual void fail (int s = HTTP_BAD_REQUEST);
  virtual void too_many_fds () { fail (); }
  virtual void fail2 (int s) {}
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
  int _bytes_recv, bytes_sent;
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
  bool _timed_out;
  bool _no_more_read;
  bool _delayed_close;

public:
  ptr<bool> destroyed_p;
  void hit_timeout () 
  { 
    _timed_out = true;
    fail (HTTP_TIMEOUT); 
  }
};

struct demux_data_t {
  demux_data_t (int p, bool s, const str &i)
    : _port (p), _ssl (s), _ssl_info (i) {}

  int _port;
  bool _ssl;
  str _ssl_info;
};

// for parent dispatcher, which will send fd's
class ahttpcon_dispatch : public ahttpcon
{
public:
  ahttpcon_dispatch (int f, int mb = SUIOLITE_DEF_BUFLEN) 
    : ahttpcon (f, NULL, mb) {}
  static ptr<ahttpcon_dispatch> alloc (int f, int mb = SUIOLITE_DEF_BUFLEN)
  { return New refcounted<ahttpcon_dispatch> (f, mb); }
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

  ptr<const demux_data_t> demux_data () const { return _demux_data; }
  ptr<demux_data_t> demux_data () { return _demux_data; }
  void set_demux_data (ptr<demux_data_t> d) { _demux_data = d; }

protected:
  void recvd_bytes (int n);
  void fail2 (int s);

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
  timecb_t *dcb;

  ptr<demux_data_t> _demux_data;

};

struct ahttp_tab_node_t {
  ahttp_tab_node_t (ahttpcon *a, ptr<bool> d)
    : _a (a), _destroyed_p (d) {}

  ahttpcon *_a;
  ptr<bool> _destroyed_p;
  tailq_entry<ahttp_tab_node_t> _qent;
};


class ahttp_tab_t {
public:
  ahttp_tab_t (int i) : interval (i), dcb (NULL), nent (0) { sched (); }
  ~ahttp_tab_t () { if (dcb) timecb_remove (dcb); }
  
  void unreg (ahttp_tab_node_t *n);
  void reg (ahttpcon *a, ptr<bool> destroyed);
  void run ();
  void sched ();
  inline size_t n_entries () const { return nent; }

private:
  const int interval;
  timecb_t *dcb;
  tailq<ahttp_tab_node_t, &ahttp_tab_node_t::_qent> q;
  size_t nent;
};


#endif /* _LIBAHTTP_AHTTP */
