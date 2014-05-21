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

static void
status_cb (cbv::ptr fail_cb, hlp_status_t st)
{
  if (st != HLP_STATUS_OK) {
    warn << "oklogd unexpected status change to code=" << int (st) << "\n";
  }

  if (st == HLP_STATUS_HOSED && fail_cb) {
    (*fail_cb) ();
  }
}

//-----------------------------------------------------------------------

tamed void
log_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
  }
  twait { h->connect (mkevent (ok)); }
  if (ok) {
    h->set_status_cb (wrap (status_cb, fail_cb));
  }
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
  if (ok) {
    he->set_status_cb (wrap (status_cb, fail_cb));
  }
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

fast_log_t::fast_log_t (int fd, str f, size_t hiwat)
  : log_t (New helper_fd_t (oklog_program_1, fd, "oklogd", 
			    HLP_OPT_PING|HLP_OPT_NORETRY)),
    _fmt (f), 
    _curr (NULL),
    _hi_wat (hiwat ? hiwat : ok_log_hiwat),
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
    size_t count (0);
  }
  df = _destroyed;

  nsec = ok_log_tick * 1000 * 100;
  while (go) {
    twait { delaycb (0, nsec, mkevent ()); }
    if ((go = !*df)) { 
      stamp_time ();
      count = (count + 1) % ok_log_period;
      if (_connected && count == 0) {
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
		 http_response_base_t *res, const str &aux, str real_ip)
{
  int s = res->get_status ();
  if (s >= 400) { // errors are 400 and above!
    add_error (x, req, res, aux, real_ip);
  }
  add_access (x, req, res, real_ip);
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
		       http_response_base_t *res, const str &aux, str real_ip)
{
  logbuf_t error;
  str ip;
  ip = (real_ip) ? real_ip : x->get_remote_ip ();
  error.log (ip);
  error.spc ();
  error.log (progname);
  error.spc ();
  error.buf (_timebuf, _timelen);
  error.spc ();
  str auxstr = aux;
  if (!auxstr && req) auxstr = req->get_target (); 
  error.log (auxstr);
  error.spc ();
  error << res->get_status ();
  add_entry (error, OKLOG_ERROR);
}

//-----------------------------------------------------------------------

void
fast_log_t::add_ssl (const str &ip, const str &cipher, const str &msg)
{
  logbuf_t ssl;
  ssl.log (ip);
  ssl.spc ();
  ssl.buf (_timebuf, _timelen);
  ssl.spc ();
  ssl.log (cipher);
  ssl.spc ();
  ssl.log (msg);
  add_entry (ssl, OKLOG_SSL);
}

//-----------------------------------------------------------------------

void 
logbuf_t::log (str s, int open, int close)
{
  if (!s || !s.len ()) { log_char ('-'); }
  else {
    if (open >= 0) { log_char (open); }
    (*this) << s;
    if (close >= 0) { log_char (close);}
  }
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
  else { ret = _curr = New oklog_arg_t (); }
  return ret;
}

//-----------------------------------------------------------------------

tamed void
fast_log_t::flush (evv_t::ptr ev)
{
  tvars {
    oklog_arg_t *arg (NULL);
    bool res;
    clnt_stat err;
  }
  if ((arg = _curr) && arg->entries.size ()) {
    _curr = NULL;
    twait { RPC::oklog_program_1::oklog_log (h, arg, &res, mkevent (err)); }
    if (err) {
      strbuf b;
      b << "oklog_log: " << err;
      okdbg_warn (ERROR, b);
    } else if (!res) {
      okdbg_warn (ERROR, "oklog_log failed");
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
  oklog_entry_t e;
  e.file = f;
  str2opaque (str (b), e.data);
  a->entries.push_back (e);
  maybe_flush ();
}

//-----------------------------------------------------------------------

void
fast_log_t::add_access (ref<ahttpcon> x, http_inhdr_t *req, 
			http_response_base_t *res, str real_ip)
{
  const char *fmp = _fmt ? _fmt.cstr () : ok_access_log_fmt.cstr ();
  const char *p;
  logbuf_t access;
  bool first = true;
  str snull;
  for (p = fmp; *p; p++) {
    if (!first) { access.spc (); } 
    else { first = false; }

    switch (*p) {
    case 't':
      access.buf (_timebuf, _timelen);
      break;
    case 'r':
      access.log (req ? (*req) ["referer"] : snull);
      break;
    case 'i':
      if (real_ip) {    
        access.log (real_ip);
      } else {
        access.log (x->get_remote_ip ());
      }
      break;
    case 'u':
      access.log (req ? (*req)["user-agent"] : snull, '"', '"');
      break;
    case '1':
      access.log (req ? req->get_line1 () : snull, '"', '"');
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
      access.log (res->get_custom_log2 (), '[', ']');
      break;
    case 'R':
      access.log(req ? (*req)["x-okws-real-ip"] : snull, '"', '"');
      break;
    case 'S':
      access.log(req ? (*req)["x-okws-real-source"] : snull, '"', '"');
      break;
    case 'I':
      access.log(x->get_remote_ip());
      break;
    default:
      access.log_char (*p);
      break;
    }
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
  _timelen = strftime (_timebuf, LOG_TIMEBUF_SIZE, "[%Y-%m-%d:%T %z]", stm);
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

