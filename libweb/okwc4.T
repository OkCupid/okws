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
    str port_s;
    okws1_port_t port;
    ptr<reqargs_t> ret;
    bool ok = true;
    str filename;
    
    static rxx url_rxx ("http(s)?://([^:/]+)(:(\\d+))?(/(.*))?");
    if (url_rxx.match (url)) {
      if (url_rxx[1] && url_rxx[1] == "s") {
	https = true;
      }
      hostname = url_rxx[2];
      port = https ? DEF_HTTPS_PORT : DEF_HTTP_PORT;
      port_s = url_rxx[4];
      if (port_s && port_s.len ()) {
	if (!convertint (port_s, &port)) {
	  warn << "okwc3: cannot convert port: " << port_s << "\n";
	  ok = false;
	}
      }
      filename = okwc3::fix_url_filename (url_rxx[5]);
      
      if (ok) {
	ptr<hostargs_t> ha = New refcounted<hostargs_t> (hostname, port, https);
	ret = New refcounted<reqargs_t> (ha, filename);
      }
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
      ptr<obj_factory_t> factory;
      ptr<req_t> req;
      ptr<resp_t> resp;
      int status;
    }
    factory = f ? f : _obj_factory;
    req = factory->alloc_req (ra);
    resp = factory->alloc_resp ();

    twait { agent_t::req (req, resp, connector::cnc (mkevent (status), ev)); }
    ev->trigger (status, resp);
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
  obj_factory_t::alloc_resp ()
  { return New refcounted<resp_simple_t> (); }

  //-----------------------------------------------------------------------

  bool
  hostargs_t::eq (const hostargs_t &ha) const
  {
    return (ha._hostname == _hostname &&
	    ha._port == _port &&
	    ha._https == _https);
  }

  //-----------------------------------------------------------------------
  
};
