// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_SLAVE_H
#define _LIBPUB_SLAVE_H 1

#include "arpc.h"
#include "async.h"
#include "xpub.h"
#include "okconst.h"
#include "txa.h"
#include "txa_prot.h"

typedef enum { PSLAVE_ERR = 0,
	       PSLAVE_SLAVE = 1,
	       PSLAVE_LISTEN = 2 } pslave_status_t;

typedef enum { HLP_STATUS_NONE = 0,
	       HLP_STATUS_OK = 1,
	       HLP_STATUS_CONNECTING = 2,
	       HLP_STATUS_ERR = 3,
	       HLP_STATUS_RETRY = 4,
	       HLP_STATUS_HOSED = 5 } hlp_status_t;

#define HLP_OPT_QUEUE   (1 << 0)        // queue requests if error
#define HLP_OPT_PING    (1 << 1)        // ping helper on startup
#define HLP_OPT_NORETRY (1 << 2)        // do not try to reconnect
#define HLP_OPT_CNCT1   (1 << 3)        // connect only once on startup

#define HLP_MAX_RETRIES 100
#define HLP_RETRY_DELAY 4
#define HLP_MAX_QLEN    1000

#define HELPER_KILL     999 

typedef callback<void, ptr<axprt_stream> >::ref pubserv_cb;
typedef callback<void, hlp_status_t>::ref status_cb_t;
bool pub_server (pubserv_cb cb, u_int port);
int pub_server (pubserv_cb cb, const str &path);
pslave_status_t pub_slave  (pubserv_cb cb, u_int port = 0, 
			    pslave_status_t *s = NULL);

class helper_base_t {
public:
  helper_base_t () : txa_login_rpc (0), authtoks (NULL) {}
  virtual ~helper_base_t () {}
  virtual void connect (cbb::ptr c = NULL) = 0;
  virtual void call (u_int32_t procno, const void *in, void *out, aclnt_cb cb,
		     time_t duration = 0) = 0;
  virtual void call (u_int32_t procno, u_int id, const void *in, void *out, 
		     aclnt_cb cb, time_t duration = 0) 
  { call (procno, in, out, cb, duration); }
  virtual str getname () const = 0;
  void hwarn (const str &s) const;

  void set_txa (u_int32_t rpc, vec<str> *v) 
  { 
    txa_login_rpc = rpc; authtoks = v; 
  }

protected:

  u_int32_t txa_login_rpc;
  vec<str> *authtoks;
  
};

class helper_t : public helper_base_t {
public:
  helper_t (const rpc_program &rp, u_int o = 0) 
    : helper_base_t (), rpcprog (rp), max_retries (hlpr_max_retries), 
      rdelay (hlpr_retry_delay),
      max_qlen (hlpr_max_qlen), max_calls (hlpr_max_calls),
      retries (0), calls (0), status (HLP_STATUS_NONE), 
      opts (o), destroyed (New refcounted<bool> (false)) {}

  virtual ~helper_t ();
  virtual vec<str> *get_argv () { return NULL; }
  int getfd () const { return fd; }
  void set_max_retries (u_int i) { max_retries = i; }
  void set_retry_delay (u_int i) { rdelay = i; }
  void set_max_qlen (u_int i) { max_qlen = i; }
  void set_max_calls (u_int i) { max_calls = i; }

  virtual int get_axprt (u_int i = 0) { return -1; }

  void connect (cbb::ptr c = NULL);
  void call (u_int32_t procno, const void *in, void *out, aclnt_cb cb,
	     time_t duration = 0);

  void retry ();
  void d_retry ();
  ptr<aclnt> get_clnt () const { return clnt; }
  virtual void kill (cbv cb, ptr<okauthtok_t> auth, 
		     oksig_t s = OK_SIG_KILL) { (*cb) (); }
  bool can_retry () const { return true; }

  // status cb will be called whenevr a slave (or helper)
  // changes status
  void set_status_cb (status_cb_t c) { stcb = c; }

protected:
  void call_status_cb () { if (stcb) (*stcb) (status); }
  virtual void launch (cbb c) = 0;
  void ping (cbb c);
  void ping_cb (cbb c, ptr<bool> df, clnt_stat err);
  bool mkclnt () { return (clnt = aclnt::alloc (x, rpcprog)); }
  void eofcb ();
  void retried (bool b);
  void connected (cbb::ptr cb, ptr<bool> df, bool b);


  void process_queue ();
  void docall (u_int32_t procno, const void *in, void *out, aclnt_cb cb,
	     time_t duration = 0);
  void didcall (aclnt_cb cb, clnt_stat st);

  void login (cbb::ptr cb, ptr<bool> df);
  void logged_in (cbb::ptr cb, ptr<bool> df, ptr<txa_login_res_t> res, 
		  clnt_stat s);
  void finish_connect_2 (cbb::ptr cb, bool b);
  void finish_connect (cbb::ptr cb, ptr<bool> df, bool b);

  const rpc_program rpcprog;
  ptr<axprt> x;
  ptr<aclnt> clnt;
  int fd;

private:

  struct queued_call_t {
    queued_call_t (u_int32_t pn, const void *i, void *o, aclnt_cb c, time_t d)
      : procno (pn), in (i), out (o), cb (c), duration (d) {}
    void call (helper_t *h) { h->call (procno, in, out, cb, duration); }
    u_int32_t procno;
    const void *in;
    void *out;
    aclnt_cb cb;
    time_t duration;
  };

  u_int max_retries;
  u_int rdelay;
  u_int max_qlen;
  u_int max_calls;
  u_int retries;
  u_int calls;
  hlp_status_t status;
  vec<queued_call_t *> queue;

protected:
  u_int opts;
  ptr<bool> destroyed;
  status_cb_t::ptr stcb;
};

class helper_fd_t : public helper_t {
public:
  helper_fd_t (const rpc_program &rp, int d, const str &n = NULL,
	       u_int o = HLP_OPT_PING|HLP_OPT_NORETRY) 
    : helper_t (rp, o), name (n) { fd = d; }
  bool can_retry () const { return false; }
  str getname () const { return name ? name : str ("(null)") ;}
protected:
  void launch (cbb c);
private:
  str name;
};

class helper_exec_t : public helper_t {
public:
  helper_exec_t (const rpc_program &rp, const vec<str> &a, u_int n = 0,
		 u_int o = HLP_OPT_PING|HLP_OPT_NORETRY) 
    : helper_t (rp, o), argv (a), n_add_socks (n) {}
  helper_exec_t (const rpc_program &rp, const str &s, u_int n = 0,
		 u_int o = HLP_OPT_PING|HLP_OPT_NORETRY)
    : helper_t (rp, o), n_add_socks (n) { argv.push_back (s); }

  vec<str> *get_argv () { return &argv; }
  str getname () const { return (argv[0]); }
  void kill (cbv cb, ptr<okauthtok_t> authtok, oksig_t s = OK_SIG_KILL);
  void add_socks (u_int i) { n_add_socks += i; }
  int get_sock (u_int i = 0) const 
  { return (i < socks.size () ? socks[i] : -1); }
  rpc_ptr<int> uid;
  rpc_ptr<int> gid;
protected:
  void launch (cbb c);
private:
  vec<int> socks;
  void setprivs ();
  vec<str> argv;
  u_int n_add_socks;
};

class helper_unix_t : public helper_t {
public:
  helper_unix_t (const rpc_program &rp, const str &p, u_int o = 0) 
    : helper_t (rp, o), sockpath (p) {}
  str getname () const { return (sockpath); }
protected:
  void launch (cbb c);
private:
  str sockpath;
};

class helper_inet_t : public helper_t {
public:
  helper_inet_t (const rpc_program &rp, const str &hn, u_int p, u_int o = 0) 
    : helper_t (rp, o), hostname (hn), port (p) {}
  virtual str getname () const { return (strbuf (hostname) << ":" << port); }
  void call (u_int32_t procno, const void *in, void *out, aclnt_cb cb,
	     time_t duration = 600) {
    helper_t::call(procno, in, out, cb, duration);
  }
        
protected:
  void launch (cbb c);
  void launch_cb (cbb c, int fd);
private:
  str hostname;
  u_int port;
};

#endif /* _LIBPUB_SLAVE_H */
