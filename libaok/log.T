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

#include "ok.h"

//-----------------------------------------------------------------------

tamed void
log_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
  }
  twait { h->connect (mkevent (ok)); }
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

tamed void
log_primary_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
  }
  twait { he->connect (mkevent (ok)); }
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

fast_log_t::fast_log_t (int fd, str f, size_t hiwat)
  : log_t (New helper_fd_t (oklog_program_1, fd, "oklogd", 
			    HLP_OPT_PING|HLP_OPT_NORETRY)),
    _fmt (f), 
    _hi_wat (hiwat),
    _destroyed (New refcounted<bool> (false)),
    _connected (false)
{
  stamp_time ();
  flush_loop ();
}

//-----------------------------------------------------------------------

fast_log_t::~fast_log_t () { *_destroyed = true; }

//-----------------------------------------------------------------------

tamed void
fast_log_t::flush_loop ()
{
  tvars {
    ptr<bool> df;
    bool go (true);
    time_t nsec;
    int count (0);
    int period (2);
    
  }
  df = _destroyed;

  nsec = 250 * 1000 * 100;
  while (go) {
    twait { delaycb (0, nsec, mkevent ()); }
    if ((go = !*df)) { 
      stamp_time ();
      count = (count + 1) % period;
      if (count == 0 && _connected) {
	twait { flush (mkevent ()); }
      }
    }
  }
}

//-----------------------------------------------------------------------

tamed void
fast_log_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
  }
  _connected = false;
  twait { log_t::connect_T (mkevent (ok)); }
  _connected = ok;
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

void
fast_log_t::log (ref<ahttpcon> x, http_inhdr_t *req, 
		 http_response_base_t *res, const str &aux)
{
  int s = res->get_status ();
  if (s != HTTP_OK && s != HTTP_REDIRECT)
    add_error (x, req, res, aux);
  add_access (x, req, res);
}

//-----------------------------------------------------------------------

void
fast_log_t::log_ssl (const str &ip, const str &cipher, const str &msg)
{
  add_ssl (ip, cipher, msg);
}

//-----------------------------------------------------------------------

bool
fast_log_t::past_high_water () const
{
  return (_curr && _curr->entries.size () >= _hi_wat);
}

//-----------------------------------------------------------------------

void
fast_log_t::add_error (ref<ahttpcon> x, http_inhdr_t *req, 
		       http_response_base_t *res, const str &aux)
{
  logbuf_t error;
  error.buf (_timebuf, _timelen);
  error.spc ();
  str ip = x->get_remote_ip ();
  error << "[client " << ip << "] " << res->get_status ();
  str auxstr = aux;
  if (!auxstr && req) auxstr = req->get_target (); 
  error.log (auxstr);
  add_entry (error, OKLOG_ERROR);
}

//-----------------------------------------------------------------------

void
fast_log_t::add_ssl (const str &ip, const str &cipher, const str &msg)
{
  logbuf_t ssl;
  ssl.buf (_timebuf, _timelen);
  ssl.spc ();
  ssl << "[client " << ip << "] " << cipher;
  if (msg) {
    ssl.spc ();
    ssl << msg;
  }
  add_entry (ssl, OKLOG_SSL);
}

//-----------------------------------------------------------------------

void 
logbuf_t::log (str s)
{
  if (!s || !s.len ()) { s = "-"; }
  (*this) << s;
}

//-----------------------------------------------------------------------

void logbuf_t::log_hex (u_int64_t x) { fmt ("%" PRIx64, x); }
void logbuf_t::log_char (char c) { fmt ("%c", c); }
void logbuf_t::spc () { log_char (' '); }

//-----------------------------------------------------------------------

oklog_arg_t *
fast_log_t::get_arg ()
{
  oklog_arg_t *ret = NULL;
  if (_curr) { ret = _curr; }
  else if (_spares.size ()) { ret = _curr = _spares.pop_back (); }
  else { ret = _curr = New oklog_fast_arg2 (); }
  return ret;
}

//-----------------------------------------------------------------------

tamed void
fast_log_t::flush (evv::ptr ev)
{
  tvars {
    oklog_arg_t *arg;
    bool res;
    clnt_stat err;
  }
  if ((arg = _curr) && arg->entries.size ()) {
    _curr = NULL;
    twait { RPC::oklog_prog_1::oklog_log (h, arg, &res, mkevent (err)); }
    if (err) {
      strbuf b;
      b << "oklog_log: " << err;
      okdbg_warn (ERROR, b);
    } else if (!res) {
      okdbg_warn ("oklog_log failed");
    }
    arg->entries.setsize (0);
    _spares.push_back (arg);
  }
  if (ev) ev->trigger ();
}

//-----------------------------------------------------------------------

void
fast_log_t::maybe_flush ()
{
  if (past_high_water ()) { flush (); }
}

//-----------------------------------------------------------------------

void
fast_log_t::add_entry (const strbuf &b, oklog_file_t f)
{
  oklog_arg_t *a = get_arg ();
  assert (a);
  a->entries.push_back (oklog_entry_t (f, b));
  maybe_flush ();
}

//-----------------------------------------------------------------------

void
fast_log_t::add_access (ref<ahttpcon> x, http_inhdr_t *req, 
			http_response_base_t *res)
{
  const char *fmp = _fmt ? _fmt.cstr () : ok_access_log_fmt.cstr ();
  const char *p;
  logbuf_t access;
  bool first = true;
  for (p = fmp; *p; p++) {
    if (!first) { access.spc (); } 
    else { first = false; }

    switch (*p) {
    case 't':
      access.buf (_timebuf, _timelen);
      break;
    case 'r':
      access.log (req ? (*req) ["referer"] : static_cast<str> (NULL));
      break;
    case 'i':
      access.log (x->get_remote_ip ());
      break;
    case 'u':
      access.log (req ? (*req)["user-agent"] : static_cast<str> (NULL));
      break;
    case '1':
      access.log (req ? req->get_line1 () : static_cast<str> (NULL));
      break;
    case 's':
      access.log_dec (res->get_status ());
      break;
    case 'b':
      access.log_dec (res->get_nbytes ());
      break;
    case 'v':
      access.log (progname);
      break;
    case 'U':
      access.log_hex (res->get_uid ());
      break;
    case 'z':
      access.log_dec (res->get_inflated_len ());
      break;
    case 'q':
      access.log_dec (req ? req->get_reqno () : 0);
      break;
    case '2':
      access.log (res->get_custom_log2 ());
      break;
    default:
      access.log_char (*p);
      break;
    }
    access.spc ();
  }
  add_entry (access, OKLOG_ACCESS);
}

//-----------------------------------------------------------------------

void
logd_parms_t::decode (const str &p)
{
  ptr<cgi_t> t (cgi_t::str_parse (p));
  t->lookup ("logdir",    &logdir);
  t->lookup ("accesslog", &accesslog);
  t->lookup ("errorlog",  &errorlog);
  t->lookup ("alfmt",     &accesslog_fmt);
  t->lookup ("user",      &user);
  t->lookup ("group",     &group);
  t->lookup ("ssllog",    &ssllog);
  t->lookup ("pidfile",   &pidfile);
}

//-----------------------------------------------------------------------

str
logd_parms_t::encode () const
{
  if (enc)
    return enc;
  cgi_t ct;
  ct.insert ("logdir",    logdir)
    .insert ("accesslog", accesslog)
    .insert ("errorlog",  errorlog)
    .insert ("alfmt",     accesslog_fmt)
    .insert ("user",      user)
    .insert ("group",     group)
    .insert ("svclog",    svclog)
    .insert ("ssllog",    ssllog);
  return ((enc = ct.encode ()));
}

//-----------------------------------------------------------------------

void
fast_log_t::stamp_time ()
{
  time_t tmp = sfs_get_timenow ();
  struct tm *stm = localtime (&tmp);
  _timeline = strftime (_timebuf, LOG_TIMEBUF_SIZE, "%Y-%m-%d:%T %z", stm);
}

//-----------------------------------------------------------------------

tamed void
log_t::turn_T (evs_t ev)
{
  tvars {
    bool ok;
    clnt_stat err;
    str ret;
  }
  twait { RPC::oklog_program_1::oklog_turn (h, &ok, mkevent (err)); }
  if (err) {
    strbuf b;
    b << "RPC Error: " << err;
    ret = b;
  } else if (!ok) {
    ret = "logger failed to turn over logs";
  }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

str make_generic_http_req (const str &in)
{
  return strbuf ("GET ") << in << " HTTP/1.x";
}

//-----------------------------------------------------------------------

