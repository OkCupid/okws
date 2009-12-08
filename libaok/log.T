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

tamed void
log_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
  }
  twait { h->connect (mkevent (ok)); }
  ev->trigger (ok);
}

tamed void
rpc_log_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
    clnt_stat err;
  }
  twait { log_t::connect_T (mkevent (ok)); }
  if (ok) {
    twait { 
      RPC::oklog_program_1::oklog_get_logset (h, &logset, mkevent (err)); 
    }
    if (err) {
      warn << "when determining oklogd parameters: " << err << "\n";
      ok = false;
    }
  }
  ev->trigger (ok);
}

tamed void
log_primary_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
  }
  twait { rpc_log_t::connect_T (mkevent (ok)); }
  if (ok) {
    ok = clone_client_t::init ();
    assert (ok);
  }
  ev->trigger (ok);
} 


tamed void
fast_log_t::connect_T (evb_t ev)
{
  tvars {
    bool ok;
  }
  twait { log_t::connect_T (mkevent (ok)); }
  if (ok)
    tmr.start ();
  ev->trigger (ok);
}

void
fast_log_t::log (ref<ahttpcon> x, http_inhdr_t *req, 
		 http_response_base_t *res, const str &aux)
{
  tmr.enable ();
  int s = res->get_status ();
  if (s != HTTP_OK && s != HTTP_REDIRECT)
    add_error (x, req, res, aux);
  add_access (x, req, res);
  if (past_high_water ()) {
    flush ();
    tmr.reset ();
  }
}

void
fast_log_t::log_ssl (const str &ip, const str &cipher, const str &msg)
{
  tmr.enable ();
  add_ssl (ip, cipher, msg);
  if (past_high_water ()) {
    flush ();
    tmr.reset ();
  }
}

bool
fast_log_t::past_high_water () const
{
  return (access.past_high_water () || error.past_high_water () ||
	  ssl.past_high_water ());
}

void
fast_log_t::add_notice (oklog_typ_t x, const str &ntc)
{
  error << tmr << ' ' << x << ' ' << ntc << '\n';
}

void
fast_log_t::add_error (ref<ahttpcon> x, http_inhdr_t *req, 
		       http_response_base_t *res, const str &aux)
{
  error << tmr << ' ' << OKLOG_ERR_ERROR << ' ';
  error.copy ("[client ", 8).remote_ip (x->get_remote_ip ())
    .copy ("] ", 2).status (res->get_status ());

  str auxstr = aux;
  if (!auxstr && req) auxstr = req->get_target (); 

  if (auxstr) 
    error.copy (": ", 2).copy (auxstr);

  error.newline ();
}

void
fast_log_t::add_ssl (const str &ip, const str &cipher, const str &msg)
{
  ssl << tmr << ' ';
  ssl.copy ("[client ", 8).remote_ip (ip).copy ("] ", 2)
    .cipher (cipher);
  if (msg) {
    ssl.copy (" ", 1).msg (msg);
  }
  ssl.newline ();
}

void
fast_log_t::add_access (ref<ahttpcon> x, http_inhdr_t *req, 
			http_response_base_t *res)
{
  const char *fmp = fmt ? fmt.cstr () : ok_access_log_fmt.cstr ();
  const char *p;
  for (p = fmp; *p; p++) {
    switch (*p) {
    case 't':
      access << tmr;
      break;
    case 'r':
      access.referer (req ? (*req) ["referer"] : static_cast<str> (NULL));
      break;
    case 'i':
      access.remote_ip (x->get_remote_ip ());
      break;
    case 'u':
      access.user_agent (req ? (*req)["user-agent"] : static_cast<str> (NULL));
      break;
    case '1':
      access.req (req ? req->get_line1 () : static_cast<str> (NULL));
      break;
    case 's':
      access.status (res->get_status ());
      break;
    case 'b':
      access.nbytes (res->get_nbytes ());
      break;
    case 'v':
      access.svc (progname);
      break;
    case 'U':
      access.uid (res->get_uid ());
      break;
    case 'z':
      access.inflated_len (res->get_inflated_len ());
      break;
    case 'q':
      access.reqno (req ? req->get_reqno () : 0);
      break;
    case '2':
      access.custom2 (res->get_custom_log2 ());
      break;
    default:
      access.cchar (*p);
      break;
    }
    access.spc ();
  }
  access.newline ();
}


void
rpc_log_t::log (ref<ahttpcon> x, http_inhdr_t *req, 
		http_response_base_t *res, const str &aux)
{
  oklog_arg_t la (OKLOG_OK);
  oklog_ok_t *lo;
  if (res->get_status () == HTTP_OK) {
    lo = la.ok;
  } else {
    la.set_typ (OKLOG_ERR_ERROR);
    lo = &(la.err->log);

    if (req) la.err->aux = req->get_target ();
    else if (aux) la.err->aux = aux;
  }
  
  lo->status = res->get_status ();

  if (logset & LOG_SZ)   lo->size = res->get_nbytes ();
  if (logset & LOG_IP)   lo->ip = x->get_remote_ip ();
  if (logset & LOG_SVC)  lo->service = progname;
  if (logset & LOG_UID)  lo->uid = res->get_uid ();

  if (req) {
    if (logset & LOG_RFR)  lo->referer = (*req)["referer"];
    if (logset & LOG_REQ)  lo->req = req->get_line1 ();
    if (logset & LOG_UA)   lo->user_agent = (*req)["user-agent"];
  }

  ref<bool> b (New refcounted<bool> (true));
  h->call (OKLOG_LOG, &la, b, wrap (this, &rpc_log_t::logged, b));
}

void
rpc_log_t::logged (ptr<bool> b, clnt_stat err)
{
  if (err) 
    warn << "RPC error in logging: " << err << "\n";
  else if (!*b) 
    warn << "log attempted failed\n";
}

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

void
log_timer_t::set_timer ()
{

  dcb = delaycb (0, tm_tick * 1000000, 
		 wrap (this, &log_timer_t::timer_cb, destroyed));
}

void
log_timer_t::stop_timer ()
{
  if (dcb) {
    timecb_remove (dcb);
    dcb = NULL;
  }
}

void
log_timer_t::disable ()
{
  disable_pending = true;
}

void
log_timer_t::enable ()
{
  if (in_timer_cb) {
    if (disable_pending) disable_pending = false;
  } else {
    if (!dcb) {
      timestamp ();
      set_timer ();
    }
  }
}

void
log_timer_t::timer_cb (ptr<bool> dstry)
{
  if (*dstry)
    return;

  if (dcb) 
    dcb = NULL;
  
  in_timer_cb = true;
  if (++counter == tm_prd) {
    (*fcb) ();
    counter = 0;
  }
  in_timer_cb = false;

  timestamp ();
  if (disable_pending) {
    disable_pending = false;
  } else
    set_timer ();
}

void
log_timer_t::timestamp ()
{
  time_t tmp = sfs_get_timenow ();
  struct tm *stm = localtime (&tmp);
  timelen = strftime (buf, LOG_TIMEBUF_SIZE, "%Y-%m-%d:%T %z", stm);
}

tamed void
fast_log_t::flush_T ()
{
  tvars {
    oklog_fast_arg_t arg;
    int ai, ei, si;
    bool ok;
    clnt_stat err;
    bool c1, c2, c3;
  }

  c1 = access.to_str (&arg.access, &ai);
  c2 = error.to_str (&arg.error, &ei);
  c3 = ssl.to_str (&arg.ssl, &si);
  
  if (c1 || c2 || c3) {
    twait { RPC::oklog_program_1::oklog_fast (h, arg, &ok, mkevent (err)); }
    if (err) {
      warn << "Error in log RPC: " << err << "\n";
    } else if (!ok) {
      warn << "Log RPC returned failure\n";
    }
    access.unlock (ai);
    error.unlock (ei);
    ssl.unlock (si);
  } else {
    tmr.disable ();
  }
}

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
