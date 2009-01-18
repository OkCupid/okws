
// -*-c++-*-
/* $Id: okwc.h 1682 2006-04-26 19:17:22Z max $ */

#ifndef _LIBWEB_OKWC4_H_
#define _LIBWEB_OKWC4_H_

#include "okcgi.h"
#include "abuf.h"
#include "aparse.h"
#include "hdr.h"
#include "str.h"
#include "web_prot.h"
#include "async.h"
#include <time.h>
#include "httpconst.h"
#include "async.h"
#include "dns.h"
#include "tame.h"
#include "okwc.h"

namespace okwc4 {

  //-----------------------------------------------------------------------

  typedef okwc3::obj_factory_t obj_factory_t;
  typedef okwc3::simple_ev_t resp_ev_t;

  //-----------------------------------------------------------------------
  
  class hostargs_t {
  public:
    hostargs_t (const str &h, okws1_port_t p, bool s);
    str hostname () const { return _hostname; }
    okws1_port_t port () const { return _port; }
    bool https () const { return _https; }
    str to_str () const;
  private:
    str _hostname;
    okws1_port_t _port;
    bool _https;
  };
  
  //-----------------------------------------------------------------------
  
  class reqargs_t {
  public:
    reqargs_t (const str &u = NULL,
	       htpv_t v = 1, 
	       str post = NULL,
	       ptr<const cgi_t> ck = NULL,
	       ptr<vec<str> > eh = NULL,
	       ptr<const hostargs_t> ha = NULL);
    
    reqargs_t (ptr<const hostargs_t> ha, const str &u)
      : _url (u), _hostargs (ha) {}
    
    reqargs_t &set_url (const str &u);
    reqargs_t &set_post (const str &p);
    reqargs_t &set_outcookies (ptr<const cgi_t> c);
    reqargs_t &set_extra_headers (ptr<vec<str> > v);
    reqargs_t &add_header (const str &h);
    reqargs_t &set_hostargs (ptr<const hostargs_t> h);
    
    static ptr<reqargs_t> alloc (const str &url);
    
    static ptr<reqargs_t> 
    alloc_proxied (const str &url, const str &ph, okws1_port_t pp, bool s);
    
    virtual ~reqargs_t () {}
    
  protected:
    str _url;
    
    htpv_t _version;
    
    str _post;
    ptr<const cgi_t> _outcookies;
    ptr<vec<str> > _extra_headers;
    ptr<const hostargs_t> _hostargs;
  };
  
  //-----------------------------------------------------------------------
  
  class agent_base_t {
  public:
    agent_base_t (ptr<hostargs_t> ha) : _hostargs (ha) {}
    virtual ~agent_base_t () {}

    virtual void 
    run (ptr<reqargs_t> ra, ptr<obj_factory_t> f, resp_ev_t ev) = 0;

  protected:
    ptr<hostargs_t> _hostargs;
  };

  //-----------------------------------------------------------------------

};


#endif /* _LIBWEB_OKWC4_H_ */
