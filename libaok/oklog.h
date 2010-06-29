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
#include "okclone.h"
#include "okformat.h"

//-----------------------------------------------------------------------

class logbuf_t : public strbuf {
public:
  logbuf_t () : strbuf () {}
  void log (str s);
};

//-----------------------------------------------------------------------

class logd_parms_t {
public:
  logd_parms_t () 
    : user (ok_logd_uname), 
      group (ok_logd_gname), 
      pidfile (ok_logd_pidfile),
      svclog (true) {}
  logd_parms_t (const str &p)
    : user (ok_logd_uname), 
      group (ok_logd_gname),
      pidfile (ok_logd_pidfile)
  { decode (p); }

  void decode (const str &p);
  str encode () const;
  
  str logdir;
  str accesslog;
  str errorlog;
  str ssllog;
  str accesslog_fmt;
  str user;
  str group;
  str pidfile;
  bool svclog;
private:
  mutable str enc;
};

//-----------------------------------------------------------------------

class log_t {
public:
  log_t (helper_t *hh) : h (hh) {}
  virtual ~log_t () { delete h; }
  virtual void connect (evb_t ev) { connect_T (ev); }
  virtual void log (ref<ahttpcon> x, http_inhdr_t *req, 
		    http_response_base_t *res,
		    const str &s) = 0;
  int getfd () const { return h->getfd (); }
  virtual void clone (evi_t ev) { ev->trigger (-1); }
  virtual void turn (evs_t ev) { turn_T (ev); }
  void kill (evv_t c, ptr<okauthtok_t> tok, 
	     oksig_t s = OK_SIG_KILL) { h->kill (c, tok, s); }
protected:
  void turn_T (evs_t cb, CLOSURE);
  void connect_T (evb_t ev, CLOSURE);
  helper_t *h;
};

//-----------------------------------------------------------------------

class log_primary_t : public clone_client_t
{
public:
  log_primary_t (helper_exec_t *hh) 
    : clone_client_t (hh, OKLOG_CLONE), 
      he (hh) {}
  void clone (evi_t ev) { clone_client_t::clone (ev); }
  void connect (evb_t ev) { connect_T (ev); }
private:
  void connect_T (evb_t ev, CLOSURE);
  helper_exec_t *he;
};

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

class fast_log_t : public log_t {
public:
  fast_log_t (int fd, str f = NULL, size_t hiwat = 256)
    : log_t (New helper_fd_t (oklog_program_2, fd, "oklogd", 
			      HLP_OPT_PING|HLP_OPT_NORETRY)),
      fmt (f), tmr (wrap (this, &fast_log_t::flush)) {}
  ~fast_log_t ();
  void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_base_t *res,
	    const str &s);
  void log_ssl (const str &i, const str &c, const str &m);
  void flush() { flush_T (); }
  void connect (evb_t ev) { connect_T (ev); }
protected:
  bool past_high_water () const;
  void add_access (ref<ahttpcon> x, http_inhdr_t *req, 
		   http_response_base_t *res);
  void add_error (ref<ahttpcon> x, http_inhdr_t *req, 
		  http_response_base_t *res, const str &aux);
  void add_ssl (const str &ip, const str &cipher, const str &msg);
  void add_notice (oklog_typ_t x, const str &ntc);
  void flush_T (CLOSURE);
  void add_entry (str s, oklog_file_t f);
private:
  void connect_T (evb_t ev, CLOSURE);
  str fmt;
  log_timer_t tmr;

  vec<oklog_fast2_arg_t *> _spares;
  oklog_fast2_arg_t *_curr;
  size_t _hi_wat;
};

//-----------------------------------------------------------------------

str make_generic_http_req (const str &in);

//-----------------------------------------------------------------------

#endif /* _LIBAOK_OKLOG_H */
