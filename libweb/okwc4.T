// -*-c++-*-
#include "okwc3.h"
#include "okwc4.h"
#include "tame_connectors.h"

namespace okwc4 {

  static okwc3::dnscache_t dnscache;

  //-----------------------------------------------------------------------
  
  reqargs_t::reqargs_t (const str &u,
			str post,
			htpv_t v,
			ptr<const cgi_t> ck,
			ptr<vec<str> > eh,
			ptr<const hostargs_t> ha)
    : _url (u),
      _version (v),
      _post (post),
      _outcookies (ck),
      _extra_headers (eh),
      _hostargs (ha) {}
  
  //-----------------------------------------------------------------------
  
#define SET(v,s) do { s = v; return (*this); } while (0)
  
  reqargs_t &reqargs_t::set_url (const str &u) { SET(u, _url); }
  reqargs_t &reqargs_t::set_post (const str &p) { SET(p, _post); }
  reqargs_t &reqargs_t::set_outcookies (ptr<const cgi_t> c) 
  { SET(c, _outcookies); }
  reqargs_t &reqargs_t::set_extra_headers (ptr<vec<str> > v) 
  { SET(v, _extra_headers); }
  reqargs_t &reqargs_t::set_hostargs (ptr<const hostargs_t> h) 
  { SET(h, _hostargs); }
  reqargs_t &reqargs_t::set_content_type (const str &s) 
  { SET(s, _content_type); }
  
#undef SET
  
  //-----------------------------------------------------------------------
  
  reqargs_t &
  reqargs_t::add_header (const str &h)
  {
    if (!_extra_headers) {
      _extra_headers = New refcounted<vec<str> > ();
    }
    _extra_headers->push_back (h);
    return (*this);
  }
  
  //-----------------------------------------------------------------------
  
  str
  hostargs_t::to_str () const
  {
    str ret = _hostname;
    if (okwc3::std_port (_https) != _port) { 
      strbuf b;
      b << ret << ":" << _port; 
      ret = b;
    }
    return ret;
  }
  
  //-----------------------------------------------------------------------
  
  ptr<reqargs_t>
  reqargs_t::alloc (const str &url)
  {
    bool https = false;
    str hostname;
    okws1_port_t port;
    ptr<reqargs_t> ret;
    str filename;

    if (okwc3::parse_http_url (url, &https, &hostname, &port, &filename)) {
      ptr<hostargs_t> ha = New refcounted<hostargs_t> (hostname, port, https);
      ret = New refcounted<reqargs_t> (ha, filename);
    }
    return ret;
  }
  
  //-----------------------------------------------------------------------
  
  ptr<reqargs_t>
  reqargs_t::alloc_proxied (const str &url, const str &ph, okws1_port_t pp,
			    bool s)
  {
    ptr<hostargs_t> ha = New refcounted<hostargs_t> (ph, pp, s);
    ptr<reqargs_t> ret = New refcounted<reqargs_t> (ha, url);
    return ret;
  }
  
  //-----------------------------------------------------------------------

  ptr<reqargs_t>
  reqargs_t::alloc_proxied (const str &url, const str &proxy)
  {
    str host;
    int port;
    ptr<reqargs_t> ret;
    if (!to_hostname_and_port (proxy, &host, &port)) {
      ret = alloc_proxied (url, host, port, false);
    }
    return ret;
  }

  //-----------------------------------------------------------------------
  
  hostargs_t::hostargs_t (const str &h, okws1_port_t p, bool s)
    : _hostname (h),
      _port (p),
      _https (s) {}
  
  //-----------------------------------------------------------------------

  tamed void
  agent_get_t::get_T (ptr<reqargs_t> ra, ptr<obj_factory_t> f, resp_ev_t ev)
  {
    tvars {
      ptr<req_t> req;
      ptr<resp_t> rsp;
      int status;
    }

    if (!f) f = _obj_factory;
    req = f->alloc_req (ra);
    twait { agent_t::req (req, f, connector::cnc (mkevent (status, rsp), ev)); }
    ev->trigger (status, rsp);
  }

  //-----------------------------------------------------------------------

  str req_t::get_type () const { return _ra->_content_type; }
  const vec<str> *req_t::get_extra_headers() const {return _ra->_extra_headers;}
  htpv_t req_t::get_version () const { return _ra->_version; }
  str req_t::get_hdr_hostname () const { return _ra->_hostargs->to_str (); }
  str req_t::get_filename () const { return _ra->_url; }
  const cgi_t * req_t::get_outcookie () const { return _ra->_outcookies; }
  str req_t::get_simple_post_str () const { return _ra->_post; }

  //-----------------------------------------------------------------------

  ptr<req_t> 
  obj_factory_t::alloc_req (ptr<const reqargs_t> ra)
  { return New refcounted<req_t> (ra); }

  //-----------------------------------------------------------------------

  ptr<resp_t>
  obj_factory_t::alloc_resp (ptr<ok_xprt_base_t> x, ptr<abuf_t> a)
  { return New refcounted<resp_simple_t> (x, a); }

  //-----------------------------------------------------------------------

  bool
  hostargs_t::eq (const hostargs_t &ha) const
  {
    return (ha._hostname == _hostname &&
	    ha._port == _port &&
	    ha._https == _https);
  }

  //-----------------------------------------------------------------------

  /**
   * keepalive req
   */
  tamed void
  agent_t::req_ka (ptr<req_t> req, ptr<resp_factory_t> of, resp_ev_t ev)
  {
    tvars {
      int status (HTTP_TIMEOUT);
      ptr<agent_t> hold;
      outcome_t outc (OUTCOME_SUCC);
      ptr<oksync::pipeliner_t::handle_t> plh;
      ptr<ok_xprt_base_t> x;
      ptr<abuf_t> abuf;
      bool newone;
      ptr<resp_t> resp;
    }
    
    // Keep this object from being freed from underneath us.
    hold = mkref (this);
    
    plh = _pipeliner->init ();
    
    twait { plh->advance (connector::cnc (mkevent (), ev, &outc)); }
    
    if (outc == OUTCOME_SUCC) {
      
      twait { get_x (_x, connector::cnc (mkevent (status, x, newone), ev)); }
      
      // set the global object to use the x we just got back; others
      // later in the pipeline might want it.  we can clear it later,
      // but those who have already gotten _x get to keep their reference 
      // to it.
      if (newone) {
	_x = x;
	_abuf = abuf = New refcounted<abuf_t> (_x->alloc_abuf_src (), true);
      } else {
	abuf = _abuf;
      }
      
      if (x) {
	twait { 
	  req->make (x, _keepalive, connector::cnc (mkevent (status), ev)); 
	}

	if (status == HTTP_OK) {
	  resp = of->alloc_resp (x, abuf);
	  
	  twait { plh->advance (connector::cnc (mkevent (), ev, &outc)); }
	  
	  if (outc == OUTCOME_SUCC) {
	    twait { resp->get (connector::cnc (mkevent (status), ev)); }
	  } 
	  abuf->finish ();
	}

	if (resp->hdr ()->connection () != HTTP_CONN_KEEPALIVE) {
	  _x = NULL;
	  _abuf = NULL;
	}

      }
    }
    ev->trigger (status, resp);
  }

  //-----------------------------------------------------------------------

  void 
  agent_t::req (ptr<req_t> req, ptr<resp_factory_t> rf, resp_ev_t cb)
  {
    if (_keepalive) { req_ka (req, rf, cb); }
    else { req_oneshot (req, rf, cb); }
  }

  //-----------------------------------------------------------------------
};
