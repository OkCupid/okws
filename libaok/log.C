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

void
log_t::connect (cbb cb)
{
  h->connect (wrap (this, &log_t::connect_cb1, cb));
}

void
log_t::connect_cb1 (cbb cb, bool b)
{
  if (b)
    connect_cb3 ();
  (*cb) (b);
}

void
rpc_log_t::connect_cb1 (cbb cb, bool b)
{
  if (!b) {
    (*cb) (b);
    return;
  }
  h->call (OKLOG_GET_LOGSET, NULL, &logset, 
	   wrap (this, &rpc_log_t::connect_cb2, cb));
}

void
rpc_log_t::connect_cb2 (cbb cb, clnt_stat err)
{
  if (err) {
    warn << "when determining oklogd parameters: " << err << "\n";
  }
  connect_cb3 ();
  (*cb) (true);
}

void
log_primary_t::connect_cb3 ()
{
  assert (clone_client_t::init ());
}

void
fast_log_t::connect_cb3 ()
{
  tmr.start ();
}

void
fast_log_t::log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
		 const str &aux)
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

bool
fast_log_t::past_high_water () const
{
  return (access.past_high_water () || error.past_high_water ());
}

void
fast_log_t::add_notice (oklog_typ_t x, const str &ntc)
{
  error << tmr << ' ' << x << ' ' << ntc << '\n';
}

void
fast_log_t::add_error (ref<ahttpcon> x, http_inhdr_t *req, 
		       http_response_t *res, const str &aux)
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
fast_log_t::add_access (ref<ahttpcon> x, http_inhdr_t *req, 
			http_response_t *res)
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
    default:
      access.cchar (*p);
      break;
    }
    access.spc ();
  }
  access.newline ();
}


void
rpc_log_t::log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
	    const str &aux)
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
    .insert ("svclog",    svclog);
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
  struct tm *stm = localtime (&timenow);
  timelen = strftime (buf, LOG_TIMEBUF_SIZE, "%Y-%m-%d:%T %z", stm);
}

void
fast_log_t::flush ()
{
  oklog_fast_arg_t arg;
  int ai, ei;
  bool call = false;
  if (access.to_str (&arg.access, &ai))
    call = true;
  if (error.to_str (&arg.error, &ei)) 
    call = true;

  if (call) {
    ptr<bool> res = New refcounted<bool>;
    h->call (OKLOG_FAST, &arg, res, 
	     wrap (this, &fast_log_t::flushed, ai, ei, res));
  } else {
    tmr.disable ();
  }
}

void
log_t::turn_cb (ptr<bool> b, okrescb cb, clnt_stat err)
{
  ptr<ok_res_t> res = New refcounted<ok_res_t> ();
  if (err) {
    *res << (strbuf ("RPC Error: ") << err);
  } else if (!*b) {
    *res << strbuf ("logger failed to turn over logs");
  }
  (*cb) (res);
}

void
log_t::turn (okrescb cb)
{
  ptr<bool> b = New refcounted<bool> ();
  h->call (OKLOG_TURN, NULL, b,
	   wrap (this, &log_t::turn_cb, b, cb));
}

void
fast_log_t::flushed (int ai, int ei, ptr<bool> r, clnt_stat err)
{
  if (err) {
    warn << "Error in log RPC: " << err << "\n";
  } else if (!*r) {
    warn << "Log RPC returned failure\n";
  }
  access.unlock (ai);
  error.unlock (ei);
}
