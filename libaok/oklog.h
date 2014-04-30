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
  void log (str s, int open = -1, int close = -1);
  template<class T> void log_dec (T i) { (*this) << i; }
  void log_hex (u_int64_t x);
  void log_char (char c);
  void spc ();
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
		    const str &s, str real_ip = nullptr) = 0;
  int getfd () const { return h->getfd (); }
  virtual void clone (evi_t ev) { ev->trigger (-1); }
  virtual void turn (evs_t ev) { turn_T (ev); }
  void kill (evv_t c, ptr<okauthtok_t> tok, 
	     oksig_t s = OK_SIG_KILL) { h->kill (c, tok, s); }
  void set_fail_cb (cbv::ptr c) { fail_cb = c; }
protected:
  void turn_T (evs_t cb, CLOSURE);
  void connect_T (evb_t ev, CLOSURE);
  helper_t *h;
  cbv::ptr fail_cb;
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
  void set_fail_cb (cbv::ptr c) { fail_cb = c; }
private:
  void connect_T (evb_t ev, CLOSURE);
  helper_exec_t *he;
  cbv::ptr fail_cb;
};

//-----------------------------------------------------------------------

class fast_log_t : public log_t {
public:
  fast_log_t (int fd, str f = NULL, size_t hiwat = 0);
  ~fast_log_t ();
  void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_base_t *res,
	    const str &s, str real_ip = nullptr);
  void log_ssl (const str &i, const str &c, const str &m);
  void flush (evv_t::ptr ev = NULL, CLOSURE);
  void connect (evb_t ev) { connect_T (ev); }
  void timer_loop (CLOSURE);
protected:
  bool past_high_water () const;
  void add_access (ref<ahttpcon> x, http_inhdr_t *req, 
		   http_response_base_t *res, str real_ip);
  void add_error (ref<ahttpcon> x, http_inhdr_t *req, 
		  http_response_base_t *res, const str &aux, str real_ip);
  void add_ssl (const str &ip, const str &cipher, const str &msg);
  void add_entry (const strbuf &s, oklog_file_t f);
  void maybe_flush ();

private:
  void stamp_time ();
  void connect_T (evb_t ev, CLOSURE);
  void flush_loop (CLOSURE);
  oklog_arg_t *get_arg ();
  str _fmt;
  vec<oklog_arg_t *> _spares;
  oklog_arg_t *_curr;
  size_t _hi_wat;
  enum { LOG_TIMEBUF_SIZE = 128 };
  char _timebuf[LOG_TIMEBUF_SIZE];
  size_t _timelen;
  ptr<bool> _destroyed;
  bool _connected;
};

//-----------------------------------------------------------------------

str make_generic_http_req (const str &in);

//-----------------------------------------------------------------------

#endif /* _LIBAOK_OKLOG_H */
