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

#ifndef _LIBAOK_OKLOG_H
#define _LIBAOK_OKLOG_H

#include "okprot.h"
#include "pubutil.h"
#include "arpc.h"
#include "ahttp.h"
#include "resp.h"
#include "okerr.h"
#include "okclone.h"
#include "okformat.h"


#define LOG_TIMEBUF_SIZE   64
#define LOG_BUF_MINSIZE    0x800     // must be at least 2wice maxwrite
#define LOG_BUF_DEFSIZE    0x10000
#define LOG_BUF_MAXWRITE   0x400
#define LOG_BUF_MAXLEN     0x800000
#define LOG_BUF_TINYLEN    32
#define LOG_BUF_HIGHWAT    0x7e00   // 2^15 - 512

// Unlike regular mstr class, mstr2 can be assigned to a str and still
// retain its data.  this allows us to reuse the malloc'ed memory, as
// growing an mstr is a bit expensive.
class mstr2 : public mstr {
public:
  mstr2 (size_t n) : mstr (n), b2 (b), alloc_sz (n) {}
  void restore () { b = b2; }
  bool resize (size_t ul);
  void fullsize () { b->len = alloc_sz + 1; }
private:
  strobjptr b2;
  size_t alloc_sz;
};

class mstrs {
public:
  mstrs (u_int sz, u_int num = 2)
  {
    for (u_int i = 0; i < num; i++) {
      bufs.push_back (New mstr2 (sz));
      unlock (i);
    }
  }

  ~mstrs () { while (bufs.size ()) delete bufs.pop_front (); }
  void unlock (int i)
  {
    if (i >= 0) {
      frlst.push_back (i); 
      bufs[i]->fullsize (); 
    }
  }
  mstr2 *get (int *i);

private:
  vec<mstr2 *> bufs;
  vec<u_int> frlst;
};

class logbuf_t {
public:
  logbuf_t (u_int s = LOG_BUF_DEFSIZE, u_int h = LOG_BUF_HIGHWAT) 
    : sz (max (s, static_cast<u_int> (LOG_BUF_MINSIZE))),
      bufs (sz, 3), okfl (true), hwat (h) { assert (getbuf ()); }
  ~logbuf_t () {}

  inline logbuf_t &time (const char *c, u_int len) { return bcpy (c, len); }
  inline logbuf_t &referer (const str &s) { return hcpy (s); }
  inline logbuf_t &remote_ip (const str &i) { return copy (i); }
  inline logbuf_t &user_agent (const str &u) { return qcpy (u); }
  inline logbuf_t &req (const str &r) { return qcpy (r); }
  inline logbuf_t &svc (const str &v) { return copy (v); }
  inline logbuf_t &nbytes (u_int n) { return put (n); }
  inline logbuf_t &cchar (char c) { return put (c); }
  inline logbuf_t &status (u_int st) { return put (st); }
  inline logbuf_t &status_long (u_int st)
  { return put (st).spc ().copy (http_status[st]); }
  inline logbuf_t &uid (u_int64_t i) { return put (i); }
  inline logbuf_t &inflated_len (size_t l) { return put ((unsigned int)l); }

  inline logbuf_t &bcpy (const char *c, u_int len);
  inline logbuf_t &qcpy (const char *c, u_int len);
  inline logbuf_t &copy (const char *c, u_int len);

  inline logbuf_t &copy (const str &s) 
  { return copy (s ? s.cstr () : NULL, s ? s.len () : 0); }
  inline logbuf_t &hcpy (const str &s) 
  { return hcpy (s ? s.cstr () : NULL, s ? s.len () : 0); }
  inline logbuf_t &qcpy (const str &s) 
  { return qcpy (s ? s.cstr () : NULL, s ? s.len () : 0); }
  inline logbuf_t &bcpy (const str &s) 
  { return bcpy (s ? s.cstr () : NULL, s ? s.len () : 0); }

  inline logbuf_t &hcpy (const char *c, u_int len);
  inline logbuf_t &put (u_int64_t i);
  inline logbuf_t &put (u_int i);
  inline logbuf_t &put (int i);
  inline logbuf_t &put (char c);
  inline logbuf_t &spc () { return put (' '); }
  inline logbuf_t &newline () { return put ('\n'); }
  inline void clear () { cp = buf; okfl = true; }
  bool output (int i);
  bool ok () const { return okfl; }
  int lock ();
  void unlock (int i) { if (i >= 0) bufs.unlock (i); }
  bool to_str (str *s, int *i);
  inline bool past_high_water () const { return ((cp - buf) > int (hwat)); }
private:
  bool getbuf ();
  void clearbuf ();
  bool resize ();

  u_int sz;
  mstrs bufs;
  mstr2 *cmbuf;
  int cmbuf_i;
  char *buf, *cp, *ep;

  suio uio;
  bool okfl;
  char tiny[LOG_BUF_TINYLEN];
  u_int hwat;
};

class logd_parms_t {
public:
  logd_parms_t () 
    : user (ok_logd_uname), group (ok_logd_gname), svclog (true) {}
  logd_parms_t (const str &p)
    : user (ok_logd_uname), group (ok_logd_gname) { decode (p); }

  void decode (const str &p);
  str encode () const;
  
  str logdir;
  str accesslog;
  str errorlog;
  str accesslog_fmt;
  str user;
  str group;
  bool svclog;
private:
  mutable str enc;
};

class log_t {
public:
  log_t (helper_t *hh) : h (hh) {}
  virtual ~log_t () { delete h; }
  virtual void connect (evb_t ev) { connect_T (ev); }
  virtual void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
		    const str &s) = 0;
  int getfd () const { return h->getfd (); }
  virtual void clone (cbi cb) { (*cb) (-1); }
  virtual void turn (okrescb cb) { turn_T (cb); }
  void kill (cbv c, ptr<okauthtok_t> tok, 
	     oksig_t s = OK_SIG_KILL) { h->kill (c, tok, s); }
protected:
  void turn_T (okrescb cb, CLOSURE);
  void connect_T (evb_t ev, CLOSURE);
  helper_t *h;
};

class rpc_log_t : public log_t {
public:
  rpc_log_t (helper_t *h) : log_t (h), logset (~0) {}
  rpc_log_t (const str &p) 
    : log_t (New helper_unix_t (oklog_program_1, p, HLP_OPT_QUEUE)) {}
  void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
	    const str &s);
  virtual void connect (evb_t ev) { rpc_log_t::connect_T (ev); }
  void connect_cb1 (cbb cb, bool b);
  void connect_cb2 (cbb cb, clnt_stat err);
 
protected:
  void connect_T (evb_t ev, CLOSURE);
  
private:
  void logged (ptr<bool> b, clnt_stat err);
  u_int32_t logset;
};

class log_primary_t : public rpc_log_t, public clone_client_t
{
public:
  log_primary_t (helper_exec_t *hh) 
    : rpc_log_t (hh),
      clone_client_t (hh, OKLOG_CLONE), 
      he (hh) {}
  void clone (cbi cb) { clone_client_t::clone (cb); }
  void connect (evb_t ev) { connect_T (ev); }
private:
  void connect_T (evb_t ev, CLOSURE);
  void connect_cb3 ();
  helper_exec_t *he;
};

class log_timer_t {
public:
  log_timer_t (cbv f, u_int i = 0, u_int p = 0) 
    : fcb (f), tm_tick (i ? i : ok_log_tick), 
      tm_prd (p ? p : ok_log_period), dcb (NULL), 
    destroyed (New refcounted<bool> (false)), counter (0),
		 in_timer_cb (false), disable_pending (false) { timestamp (); }
  ~log_timer_t () { stop_timer (); *destroyed = true; }
  const char *gettime (u_int *len) const { *len = timelen; return buf; }
  void start () { set_timer (); }
  void reset () { counter = 0; }
  void enable () ;
  void disable () ;
private:
  void set_timer ();
  void timer_cb (ptr<bool> d);
  void stop_timer ();
  void timestamp ();
  cbv fcb;
  u_int tm_tick, tm_prd;
  timecb_t *dcb;
  ptr<bool> destroyed;
  u_int counter;
  char buf[LOG_TIMEBUF_SIZE];
  u_int timelen;
  bool in_timer_cb, disable_pending;
};

class fast_log_t : public log_t {
public:
  fast_log_t (int fd, const str f = NULL)
    : log_t (New helper_fd_t (oklog_program_1, fd, "oklogd", 
			      HLP_OPT_PING|HLP_OPT_NORETRY)),
      fmt (f), tmr (wrap (this, &fast_log_t::flush)) {}
  void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
	    const str &s);
  void flush() { flush_T (); }
  void connect (evb_t ev) { connect_T (ev); }
protected:
  bool past_high_water () const;
  void add_access (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res);
  void add_error (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
		  const str &aux);
  void add_notice (oklog_typ_t x, const str &ntc);
  void flush_T (CLOSURE);
private:
  void connect_T (evb_t ev, CLOSURE);
  str fmt;
  log_timer_t tmr;
  logbuf_t access, error;
};

logbuf_t &
logbuf_t::copy (const char *c, u_int len)
{
  if (len && c && cp) {
    if (len < LOG_BUF_MAXWRITE && (cp + len <= ep || resize ())) {
      memcpy (cp, c, len);
      cp += len;
      assert (cp <= ep);
    } else {
      okfl = false;
    }
  }
  return (*this);
}

logbuf_t &
logbuf_t::bcpy (const char *c, u_int len)
{
  return put ('[').copy (c, len).put (']');
}

logbuf_t &
logbuf_t::qcpy (const char *c, u_int len)
{
  return put ('\"').copy (c, len).put ('\"'); 
}

logbuf_t &
logbuf_t::put (char ch)
{
  if (cp) {
    if (cp < ep || resize ()) *cp++ = ch;
    else okfl = false;
  }
  return (*this);
}

logbuf_t &
logbuf_t::put (u_int i)
{
  u_int l = sprintf (tiny, "%u", i);
  return copy (tiny, l);
}

logbuf_t &
logbuf_t::put (u_int64_t i)
{
  u_int l = sprintf (tiny, "%" PRIx64 "", i);
  return copy (tiny, l);
}

logbuf_t &
logbuf_t::put (int i)
{
  u_int l = sprintf (tiny, "%d", i);
  return copy (tiny, l);
}

logbuf_t &log (logbuf_t *b, oklog_typ_t typ);

inline logbuf_t &operator<< (logbuf_t &lb, oklog_typ_t typ)
{
  return log (&lb, typ);
}

inline logbuf_t &operator<< (logbuf_t &lb, char c)
{
  return lb.put (c);
}

inline logbuf_t &operator<< (logbuf_t &lb, int32_t i)
{
  return lb.put (i);
}

inline logbuf_t &operator<< (logbuf_t &lb, const str &s)
{
  return lb.copy (s);
}

logbuf_t &
logbuf_t::hcpy (const char *c, u_int len)
{
  return ((c && len) ? copy (c, len) : put ('-'));
}

inline logbuf_t &operator<< (logbuf_t &lb, const log_timer_t &t)
{
  u_int len;
  const char *c = t.gettime (&len);
  return lb.bcpy (c, len);
}

inline str 
make_generic_http_req (const str &in)
{
  return strbuf ("GET ") << in << " HTTP/1.x";
}

#endif /* _LIBAOK_OKLOG_H */
