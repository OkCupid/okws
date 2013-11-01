// -*-c++-*-

#include "ok3.h"
#include "tame_connectors.h"
#include "resp2.h"

//-----------------------------------------------------------------------

okclnt3_t::req_t::req_t (okclnt3_t *o, ptr<ahttpcon> x, abuf_t *b, u_int reqno, 
			 htpv_t prev_vers, u_int to)
  : http_parser_cgi_t (x, to, b),
    _ok_clnt (o)
{
  hdr.set_reqno (reqno, true, prev_vers);
}

//-----------------------------------------------------------------------

okclnt3_t::req_t::~req_t () {}

//-----------------------------------------------------------------------

okclnt3_t::resp_t::~resp_t ()
{
}

//-----------------------------------------------------------------------

okclnt3_t::resp_t::resp_t (okclnt3_t *o, ptr<req_t> q)
  : _ok_clnt (o),
    _uid (0),
    _uid_set (false), 
    _rsp_gzip (false),
    _sent (false),
    _replied (false),
    _req (q),
    _error_ok (false),
    _serving (true),
    _add_connection (true) {}

//-----------------------------------------------------------------------

okclnt3_t::okclnt3_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to)
  : okclnt_interface_t (o),
    _x (xx),
    _abuf (New abuf_t (New abuf_con_t (_x), true)),
    _timeout (to),
    _union_cgi_mode (false),
    _serving (false) {}

//-----------------------------------------------------------------------

okclnt3_t::~okclnt3_t ()
{
  for (size_t i = 0; i < _resps.size(); i++) {
    _resps[i]->mark_defunct ();
  }
  delete _abuf;
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::reply (int st, ptr<compressible_t> b, str url, str es)
{
  assert (!_replied);
  if (_ok_clnt) {
    _status = st;
    _body = b;
    _redir_url = url;
    _error_str = es;
    _replied = true;

    _ok_clnt->poke ();
  }
}

//-----------------------------------------------------------------------

str
okclnt3_t::ssl_cipher () const 
{
  if (_demux_data) return _demux_data->ssl_info ();
  else return NULL;
}

//-----------------------------------------------------------------------

void
okclnt3_t::set_localizer (ptr<const pub3::localizer_t> l)
{
  // Clone the service object's remote publisher....
  ptr<pub3::remote_publisher_t> r = 
    New refcounted<pub3::remote_publisher_t> (*oksrvc->pub3 ());

  // In our own private copy, set the localizer...
  r->set_localizer (l);

  // And store it away in our slot for a generic publisher...
  _p3_locale = r;
}

//-----------------------------------------------------------------------

ptr<pub3::ok_iface_t>
okclnt3_t::pub3 ()
{
  return oksrvc->pub3 ();
}

//-----------------------------------------------------------------------

ptr<pub3::ok_iface_t>
okclnt3_t::pub3_local ()
{
  if (!_p3_locale) { _p3_locale = pub3 (); }
  return _p3_locale;
}

//-----------------------------------------------------------------------

void
okclnt3_t::redirect (int status, const str &u)
{
  alloc_resp ()->redirect (status, u);
  poke ();
}

//-----------------------------------------------------------------------

void
okclnt3_t::error (int status, ptr<req_t> req)
{
  alloc_resp (req)->error (status);
  poke ();
}

//-----------------------------------------------------------------------

bool
okclnt3_t::check_ssl ()
{
  str s;
  bool ok (true);

  if (ssl_only () && !is_ssl ()) {
    if ((s = ssl_redirect_str ())) redirect (HTTP_MOVEDPERM, s);
    else error (HTTP_FORBIDDEN);
    ok = false;
  }
  return ok;
}

//-----------------------------------------------------------------------

ptr<okclnt3_t::resp_t>
okclnt3_t::alloc_resp (ptr<req_t> req)
{
  ptr<resp_t> r = New refcounted<resp_t> (this, req);
  _resps.push_back (r);
  return r;
}

//-----------------------------------------------------------------------

ptr<okclnt3_t::req_t>
okclnt3_t::alloc_req (u_int rn, htpv_t prev)
{
  u_int to = (rn > 1 ? okclnt3::keepalive_timeout : ok_clnt_timeout);
  return New refcounted<req_t> (this, _x, _abuf, rn, prev, to);
}

//-----------------------------------------------------------------------

static bool
is_clean_eof (int status)
{
  return (status == HTTP_PIPELINE_CLEAN_TIMEOUT || status == HTTP_PIPELINE_EOF);
}


//-----------------------------------------------------------------------

tamed void
okclnt3_t::serve_T ()
{
  tvars {
    ptr<okclnt3_t::req_t> req;
    ptr<okclnt3_t::resp_t> resp;
    int status;
    rendezvous_t<> rv (__FILE__, __LINE__);
    u_int reqno (1);
    htpv_t prev (0);
  }

  _serving = true;

  output_loop (ok_clnt_timeout, mkevent (rv));

  _serving = check_ssl ();

  while (_serving) {

    req = alloc_req (reqno++, prev);

    twait { req->parse (mkevent (status, _serving)); }

    if (status == HTTP_OK) {
      prev = req->http_vers ();
      resp = alloc_resp(req);
      process (req, resp, mkevent (rv));
    } else if (!is_clean_eof (status)) {
      error (status, req);
    }

    if (resp && !resp->keep_serving()) {
        _serving = false;
    }

    if (okclnt3::n_keepalives >= 0 && int (reqno) > okclnt3::n_keepalives) {
      _serving = false;
    }
  }

  // always poke when leaving the loop, to get the output loop
  // to make sure to flush.  not always needed, but can't hurt.
  poke ();

  while (rv.need_wait ()) {
    twait (rv);
  }

  finish_serve ();
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::req_t::parse (okclnt3_t::req_t::parse_ev_t ev)
{
  tvars {
    int status;
    bool go;
  }

  twait { http_parser_cgi_t::parse (mkevent (status)); }
  go = (status == HTTP_OK && 
	hdr.get_conn_mode () == HTTP_CONN_KEEPALIVE &&
	(hdr.mthd == HTTP_MTHD_GET || hdr.mthd == HTTP_MTHD_HEAD) &&
	hdr.get_vers () >= 1);
  ev->trigger (status, go);
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::wait_for_ready_output (okclnt3_t::resp_t::ev_t ev)
{
  tvars {
    ptr<okclnt3_t::resp_t> ret;
    bool go (true);
  }

  while (go) {

    if (!ret && _resps.size ())  {
      ret = _resps.pop_front ();
    }

    if (ret && ret->is_ready ()) {
      go = false;
    } else if (!ret && !_serving) {
      go = false;
    } else {
      twait { _output_cv.wait (mkevent ()); }
    }

  }

  ev->trigger (ret);
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::output_loop (int time_budget, evv_t ev)
{
  tvars {
    ptr<okclnt3_t::resp_t> rsp;
    time_t start, stop;
    bool go (true);
    int fail_status (HTTP_OK);
  }

  while (go) {

    twait { wait_for_ready_output (mkevent (rsp)); }

    if (!rsp) {
      go = false;

    } else {

      assert (rsp->is_ready ());
    
      start = sfs_get_timenow ();
      twait { rsp->send (mkevent (go), time_budget, &fail_status); }
      stop = sfs_get_timenow ();
      
      // update the time budget for next time around....
      time_budget = max<int> (1, time_budget - (stop - start));

    }
  }

  // Cancel all of these guys.
  while (_resps.size ()) {
    assert (fail_status != HTTP_OK);
    rsp = _resps.pop_front ();
    twait { rsp->cancel (fail_status, mkevent ()); }
  }

  ev->trigger ();
}

//-----------------------------------------------------------------------

gzip_mode_t
okclnt3_t::resp_t::do_gzip () const
{
  gzip_mode_t ret = GZIP_NONE;
  if (_req && 
      _req->hdr_cr ().takes_gzip () && 
      ok_gzip_mode != GZIP_NONE &&
      _body &&
      _body->inflated_len () &&
      _rsp_gzip) {

    ret = ok_gzip_mode;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::set_connection_attributes (http_resp_attributes_t *hra)
{
  if (_add_connection) {
      str tmp;
      if (_req) {
          tmp = _req->hdr_cr ().get_connection ();
      }

      if (tmp) hra->set_connection (tmp);
  }
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::set_error_attributes (http_resp_attributes_t *hra)
{
  set_connection_attributes (hra);
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::set_attributes (http_resp_attributes_t *hra)
{
  if (_cache_control) hra->set_cache_control (_cache_control);
  if (_content_type) hra->set_content_type (_content_type);
  if (_expires) hra->set_expires (_expires);
  if (_cont_disp) hra->set_content_disposition (_cont_disp);

  set_connection_attributes (hra);

  // set anything else you want
  hra->set_others (_hdr_fields);

  browser_specific_fixups (req ()->hdr_cr(), hra);
}

//-----------------------------------------------------------------------

oksrvc_t *
okclnt3_t::resp_t::svc ()
{
  oksrvc_t *ret = NULL;
  if (_ok_clnt) ret = _ok_clnt->get_oksrvc ();
  return ret;
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::resp_t::send (evb_t ev, time_t time_budget, int *nxt_status_p)
{
  tvars {
    ptr<http_response_base_t> rsp;
    holdvar ptr<okclnt3_t::req_t> req (_self->req ());
    http_inhdr_t *inhdrs (req ? req->hdr_p () : NULL);
    holdvar int vers (req ? req->hdr_cr ().get_vers () : 0);
    holdvar http_method_t meth (req->hdr_cr ().mthd);
    http_resp_attributes_t hra (_self->status (), vers, meth);
    gzip_mode_t gz (_self->do_gzip ());
    bool ok (true);
    oksrvc_t *svc (_self->svc ());
    ptr<ahttpcon> x (_self->con ());
    outcome_t outc (OUTCOME_SUCC);
    int nxt_status;
    ssize_t rc;
    compressible_t::opts_t opts;
  }

  hra.set_others (_hdr_fields);

  if (!x || x->closed ()) {
    nxt_status = _status = HTTP_CLIENT_EOF;
    ok = false;
  }

  // Handle Redirects
  if (_redir_url)  {
    if (httpconst::is_redirect (_status)) {
      rsp = New refcounted<http_response_redirect_t> (_redir_url, hra);
    } else {
      _status = HTTP_SRV_ERROR;
      hra.set_status (_status);
    }
  }

  // Handle Errors
  if (!_error_ok && httpconst::is_error (_status)) {
    set_error_attributes (&hra);
    if (svc) {
      twait { svc->geterr (_error_str, hra, mkevent (rsp)); }
    } else {
      nxt_status = HTTP_SRV_ERROR;
      ok = false;
    }

  } else if (!rsp) {

    // Handle the case in which the response is OK to send out,
    // but the response must be generated...

    set_attributes (&hra);
    opts = compressible_t::opts_t (gz, hra.get_chunking_support());
    hra.set_content_delivery (opts);
    rsp = New refcounted<http_response_ok2_t> (hra, _body);
  }

  // now make a final pass
  if (_uid_set) rsp->set_uid (_uid);

  if (_error_ok || !httpconst::is_error (_status)) { 
    fixup_cookies (rsp); 
  }

  if (x && ok) {

    twait { 
      rsp->send2 (x, connector::timeout (mkevent (rc), time_budget, 0, &outc));
    }
    
    if (outc == OUTCOME_TIMEDOUT) {
      str i = x->all_info ();
      ok = false;
      nxt_status = HTTP_TIMEOUT;
      warn << "timeout in send: " << i << "\n";
      x->cancel ();
    }
  }

  if (_log_fixup_cb) { 
    (*_log_fixup_cb) ();
  }
  fixup_log (rsp);
  svc->log (x, inhdrs, rsp);
  
  if (nxt_status_p)
    *nxt_status_p = nxt_status;
  
  // tell the output loop it can keep on going
  ev->trigger (ok);
  
  // tell the developer-specified process() function that it's OK to
  // release the memory associated with sending this reply.
  do_release_ev ();
}

//-----------------------------------------------------------------------

void
okclnt3_t::rrpair_t::set_log_fixup_cb (cbv::ptr cb)
{
  if (_resp) { 
    _resp->set_log_fixup_cb (cb); 
  }
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::do_release_ev ()
{
  if (_release_ev) {
    evv_t ev (_release_ev);
    _release_ev = NULL;
    ev->trigger ();
  }
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::resp_t::cancel (int status, evv_t ev)
{
  tvars {
    bool dummy;
  }

  _redir_url = NULL;
  _status = status;
  _body = NULL;

  twait { send (mkevent (dummy), 0); }
  ev->trigger ();
}

//-----------------------------------------------------------------------

void
okclnt3_t::poke ()
{
  _output_cv.poke ();
}

//-----------------------------------------------------------------------

ptr<ahttpcon>
okclnt3_t::resp_t::con ()
{
  ptr<ahttpcon> ret;
  if (_ok_clnt) { ret = _ok_clnt->con (); }
  return ret;
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::fixup_log (ptr<http_response_base_t> rsp)
{
  if (_custom_log2) rsp->set_custom_log2 (_custom_log2);
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::set_hdr_field (const str &k, const str &v)
{
  if (!_hdr_fields)
    _hdr_fields = New refcounted<vec<http_hdr_field_t> > ();
  _hdr_fields->push_back (http_hdr_field_t (k,v));
}

//-----------------------------------------------------------------------

void
okclnt3_t::rrpair_t::okreply (ptr<compressible_t> c, evv_t::ptr ev)
{ 
  _resp->set_release_ev (ev);
  _resp->reply (HTTP_OK, c); 
} 

//-----------------------------------------------------------------------

void
okclnt3_t::rrpair_t::redirect (const str &u, int s, evv_t::ptr ev)
{
  _resp->set_release_ev (ev);
  _resp->redirect (s > 0 ? s : HTTP_MOVEDPERM, u);
}

//-----------------------------------------------------------------------

void
okclnt3_t::rrpair_t::error (int st, const str &s, evv_t::ptr ev)
{
  _resp->set_release_ev (ev);
  _resp->error (st, s);
}

//-----------------------------------------------------------------------

ptr<demux_data_t>
okclnt3_t::rrpair_t::demux_data () 
{
  ptr<demux_data_t> d;
  okclnt3_t *o;
  if (_req && (o = _req->ok_clnt ())) 
    d = o->demux_data ();
  return d;
}

//-----------------------------------------------------------------------

ptr<const demux_data_t>
okclnt3_t::rrpair_t::demux_data () const
{
  ptr<const demux_data_t> d;
  const okclnt3_t *o;
  if (_req && (o = _req->ok_clnt ())) 
    d = o->demux_data ();
  return d;
}

//-----------------------------------------------------------------------

// Constants for dealining with keepalives
namespace okclnt3 {
  int n_keepalives = 20;
  int keepalive_timeout = 20;
};

//-----------------------------------------------------------------------
