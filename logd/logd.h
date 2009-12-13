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
#ifndef _LOGD_LOGD_H
#define _LOGD_LOGD_H

#include "ok.h"
#include "okprot.h"
#include "list.h"
#include "okclone.h"

class logfile_t {
public:
  logfile_t (const str &n) : fn (n), fd (-1) {}
  logfile_t () : fd (-1) {}
  void setfile (const str &n) { fn = n; }
  ~logfile_t () { close (); }
  logbuf_t *getbuf () { return &buf; }
  bool open (const str &n);
  bool open_verbose (const str &n, const str &typ);
  void flush () { if (fd >= 0) buf.output (fd); }
  bool flush (const str &s);
  void close ();
private:
  logbuf_t buf;
  str fn;
  int fd;
};

class logd_fmt_el_t {
public:
  logd_fmt_el_t () {}
  virtual ~logd_fmt_el_t () {}
  virtual u_int get_switch () const { return 0; }
  virtual void log (logbuf_t *b, const oklog_ok_t &x) = 0;
};

class logd_fmt_referer_t : public logd_fmt_el_t  {
public:
  logd_fmt_referer_t () : logd_fmt_el_t () {}
  u_int get_switch () const { return LOG_RFR; }
  void log (logbuf_t *b, const oklog_ok_t &x) { b->referer (x.referer); }
};

class logd_fmt_remote_ip_t : public logd_fmt_el_t {
public:
  logd_fmt_remote_ip_t () : logd_fmt_el_t () {}
  u_int get_switch () const { return LOG_IP; }
  void log (logbuf_t *b, const oklog_ok_t &x) { b->remote_ip (x.ip); }
};

class logd_fmt_ua_t  : public logd_fmt_el_t {
public:
  logd_fmt_ua_t () : logd_fmt_el_t () {}
  u_int get_switch () const { return LOG_UA; }
  void log (logbuf_t *b, const oklog_ok_t &x) { b->user_agent (x.user_agent); }
};

class logd_fmt_req_t : public logd_fmt_el_t {
public:
  logd_fmt_req_t () : logd_fmt_el_t () {}
  u_int get_switch () const { return LOG_REQ; }
  void log (logbuf_t *b, const oklog_ok_t &x) { b->req (x.req); }
};

class logd_fmt_status_t : public logd_fmt_el_t {
public:
  logd_fmt_status_t () : logd_fmt_el_t () {}
  void log (logbuf_t *b, const oklog_ok_t &x) { b->status (x.status); }
};

class logd_fmt_bytes_t : public logd_fmt_el_t {
public:
  logd_fmt_bytes_t () : logd_fmt_el_t () {}
  u_int get_switch () const { return LOG_SZ; }
  void log (logbuf_t *b, const oklog_ok_t &x) { b->nbytes (x.size); }
};

class logd_fmt_svc_t : public logd_fmt_el_t {
public:
  logd_fmt_svc_t () : logd_fmt_el_t () {}
  u_int get_switch () const { return LOG_SVC; }
  void log (logbuf_t *b, const oklog_ok_t &x) { b->svc (x.service); }
};

class logd_fmt_uid_t : public logd_fmt_el_t {
public:
  logd_fmt_uid_t () : logd_fmt_el_t () {}
  u_int get_switch () const { return LOG_UID; }
  void log (logbuf_t *b, const oklog_ok_t &x) { b->uid (x.uid); }
};

class logd_fmt_const_t : public logd_fmt_el_t {
public:
  logd_fmt_const_t (char c) : logd_fmt_el_t (), ch (c) {}
  void log (logbuf_t *b, const oklog_ok_t &x) { b->cchar (ch); }
private:
  char ch;
};

class logd_t;
class logd_client_t {
public:
  logd_client_t (ptr<axprt_stream> x, logd_t *d, bool p);
  ~logd_client_t ();
  void dispatch (svccb *sbp);
  tailq_entry<logd_client_t> lnk;
private:
  ptr<asrv> srv;
  logd_t *logd;
  bool primary;
};

class logd_t : public clone_server_t {
public:
  logd_t (ptr<axprt_unix> x, const str &in)
    : clone_server_t (x),
      tmr (wrap (this, &logd_t::flush)), 
      parms (in), logset (0), error (NULL), access (NULL), ssl (NULL),
      dcb (NULL), 
      uid (getuid ()), usr (parms.user), grp (parms.group), running (false),
      injail (false) {}

  void launch ();
  void remove (logd_client_t *c) { lst.remove (c); }
  void newclnt (bool p, ptr<axprt_stream> x);
  void dispatch (svccb *sbp);
  void shutdown ();
  void handle_sighup ();

  log_timer_t tmr;

protected:

  // need to implement this to be a clone server
  void register_newclient (ptr<axprt_stream> x) { newclnt (false, x); }

private:
  void turn (svccb *sbp);
  bool turn ();
  void log (svccb *sbp);
  void fastlog (svccb *sbp);
  void flush ();
  void timer_setup () { tmr.start (); }
  bool setup ();
  bool slave_setup ();
  bool pidfile_setup ();
  bool logfile_setup () ;
  bool logfile_setup (logfile_t **f, const str &l, const str &t);
  bool perms_setup ();
  void close_logfiles ();
  void parse_fmt ();
  bool access_log (const oklog_ok_t &x);
  bool error_log (const oklog_arg_t &x);
  bool ssl_log (const oklog_ssl_msg_t &x);
  void clean_pidfile ();
  str fixup_file (const str &in) const;

  tailq<logd_client_t, &logd_client_t::lnk> lst;
  vec<logd_fmt_el_t *> fmt_els;
  logd_parms_t parms;
  u_int logset;
  logfile_t *error, *access, *ssl;
  timecb_t *dcb;
  int uid;
  ok_usr_t usr;
  ok_grp_t grp;
  bool running;

  bool injail;
  str _pidfile;
  
};

class logd_fmt_time_t : public logd_fmt_el_t {
public:
  logd_fmt_time_t (logd_t *d) : logd_fmt_el_t (), logd (d) {}
  void log (logbuf_t *b, const oklog_ok_t &t) { (*b) << logd->tmr; }
private:
  logd_t *logd;
};



#endif /* _LOGD_LOGD_H */
