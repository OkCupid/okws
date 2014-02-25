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
#include "tame.h"

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

//=======================================================================

class cbv_countdown_t : public virtual refcount {
public:
  cbv_countdown_t (cbv c) : cb (c) {}
  void reset (cbv c) { cb = c; }
  ~cbv_countdown_t () { (*cb) (); }
private:
  cbv cb;
};

class abuf_src_t;

//=======================================================================

class ok_xprt_base_t : public virtual refcount {
public:
  ok_xprt_base_t () {}
  virtual ~ok_xprt_base_t () {}
  virtual abuf_src_t *alloc_abuf_src () = 0;
  virtual void drain_to_network (ptr<strbuf> b, evb_t ev) = 0;
  virtual void drain_cancel () = 0;
  virtual bool ateof () const = 0;
};

//=======================================================================

class byte_counter_t {
public:
  byte_counter_t () : _n_sent (0), _n_recv (0) {}
  void update (ssize_t s, ssize_t r);
  void clear ();
  void query_and_clear (size_t *s, size_t *r);
  void query (size_t *s, size_t *r);
  size_t get_bytes_sent () const { return _n_sent; }
  size_t get_bytes_recv () const { return _n_recv; }
private:
  size_t _n_sent, _n_recv;
};

extern byte_counter_t ahttpcon_byte_counter;

//=======================================================================

struct keepalive_data_t {
  keepalive_data_t (int reqno = 0, const char *buf = NULL, size_t l = 0)
    : _reqno (reqno), _buf (buf), _len (l) {}
  void inc_reqno () { _reqno++; }
  int _reqno;
  const char *_buf;
  size_t _len;
};

//=======================================================================

class ahttpcon : public ok_xprt_base_t
{
protected:

  // Keep track of what state this ahttpcon is in, so we can
  // debug the zombies that are hanging around for too long.
  typedef enum { AHTTPCON_STATE_NONE = 0,
		 AHTTPCON_STATE_DEMUX = 1,
		 AHTTPCON_STATE_RECV = 2,
		 AHTTPCON_STATE_SEND = 3,
		 AHTTPCON_STATE_SEND2 = 4,
		 AHTTPCON_STATE_CLOSED = 5 } state_t; 
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

  //--------------------------------------------------
  // ok_xprt_base_t interface
  abuf_src_t *alloc_abuf_src () ;
  void drain_to_network (ptr<strbuf> b, evb_t ev);
  void drain_cancel ();
  bool ateof () const { return eof; }
  
  //--------------------------------------------------

  int getfd () const { return fd; }
  virtual int takefd ();
  inline sockaddr_in *get_sin () const { return sin; }
  inline const str & get_remote_ip () const { return remote_ip; }
  inline int get_remote_port () const { return _remote_port; }
  hash_t source_hash () const;
  hash_t source_hash_ip_only () const;
  virtual ~ahttpcon ();
  void setrcb (cbi::ptr cb); // cb called when reading regular byte streams
  void seteofcb (cbv::ptr c) { eofcb = c; }
  void output (ptr<bool> destroyed_local);
  void spacecb ();
  void error (int ec);
  void send (const strbuf &b, cbv::ptr drained, cbv::ptr sent = NULL);
  void sendv (const iovec *iov, int cnt, cbv::ptr drained = NULL,
	      cbv::ptr sent = NULL);
  void send2 (const strbuf &b, event<ssize_t>::ref ev, CLOSURE);
  void copyv (const iovec *iov, int cnt);
  suiolite *uio () const { return in; }
  size_t get_bytes_sent () const { return bytes_sent; }
  size_t get_bytes_recv () const { return _bytes_recv; }

  size_t set_keepalive_data (const keepalive_data_t &d);
  u_int get_reqno () const { return _reqno; }

  str select_set () const;
  str all_info () const;
  virtual str get_debug_info () const { return NULL; }

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
  void cancel () { fail(); }
  void stop_read ();
  void short_circuit_output ();
  int bytes_recv () const { return _bytes_recv;}

  template<size_t n> void
  collect_scraps (rpc_bytes<n> &out) { in->load_into_xdr<n> (out); }
  
  const time_t start;

protected:
  void set_remote_ip ();
  virtual int dowritev (int cnt) { return out->output (fd, cnt); }
  virtual ssize_t doread (int fd);
  virtual void recvd_bytes (size_t n);
  virtual void fail ();
  virtual void read_fail (int s) { fail (); }
  virtual void too_many_fds () { fail (); }
  virtual void fail2 () {}
  void input (ptr<bool> destroyed_local);
  bool enable_selread ();
  void disable_selread ();
  void call_drained_cb ();
  void zombie_warn (ptr<bool> df);

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
  timecb_t *_zombie_tcb;
  state_t _state;

public:
  rpc_bytes<> request_bytes;
  ptr<bool> destroyed_p;
  void kill ();
  void hit_timeout ();

private:
  int _remote_port;
  mutable hash_t _source_hash;
  mutable hash_t _source_hash_ip_only;

protected:
  u_int _reqno; // for Keep-Alive, this is incremented once-per
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

// Keeps a copy of data received before the fd is passed
typedef callback<void, str, int>::ref clonecb_t;
class ahttpcon_clone : public ahttpcon
{
public:
  ahttpcon_clone (int f, sockaddr_in *s = NULL, size_t ml = AHTTP_MAXLINE);
  void setccb (clonecb_t cb, size_t nplb);
  int takefd ();

  static ptr<ahttpcon_clone> 
  alloc (int f, sockaddr_in *s = NULL, size_t ml = AHTTP_MAXLINE) 
  { return New refcounted<ahttpcon_clone> (f, s, ml); }

  void declone ();

protected:
  void recvd_bytes (size_t n);
  void fail2 ();
  void read_fail (int s);
  void issue_ccb (int s);

private:
  void end_read ();
  str delimit (int *delimit_status);

  const size_t maxline;
  clonecb_t::ptr ccb;

  bool decloned;
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

  ahttp_tab_t (int i) 
    : interval (i), dcb (NULL), nent (0), _shutdown (false)
  { sched (); }

  ~ahttp_tab_t () { if (dcb) timecb_remove (dcb); }
  
  void unreg (ahttp_tab_node_t *n);
  void reg (ahttpcon *a, ptr<bool> destroyed);
  void run ();
  void sched ();
  void shutdown ();
  void kill_all ();
  inline size_t n_entries () const { return nent; }

private:
  const int interval;
  timecb_t *dcb;
  tailq<ahttp_tab_node_t, &ahttp_tab_node_t::_qent> q;
  size_t nent;
  bool _shutdown;
};


#endif /* _LIBAHTTP_AHTTP */
