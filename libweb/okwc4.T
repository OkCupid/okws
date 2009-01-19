// -*-c++-*-
#include "okwc3.h"
#include "okwc4.h"

namespace okwc4 {

  static okwc3::dnscache_t dnscache;

  //-----------------------------------------------------------------------
  
  reqargs_t::reqargs_t (const str &u,
			htpv_t v, str post,
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
  
  hostargs_t::hostargs_t (const str &h, okws1_port_t p, bool s)
    : _hostname (h),
      _port (p),
      _https (s) {}
  
  //-----------------------------------------------------------------------

  tamed void
  agent_oneshot_t::run_T (ptr<reqargs_t> ra, ptr<obj_factory_t> f, resp_ev_t ev)
  {



  }

  //-----------------------------------------------------------------------
  
};
