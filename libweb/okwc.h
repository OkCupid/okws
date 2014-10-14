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

#ifndef _LIBWEB_OKWC_H
#define _LIBWEB_OKWC_H

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
#include "inhdr.h"

//
// okwc = OK Web Client
//

//-----------------------------------------------------------------------

class okwc_dnscache_entry_t : public virtual refcount {
public:
  okwc_dnscache_entry_t (const str &h, int t = 60) : 
    hostname (h), expires (0), resolving (false), ttl (t), err (0), 
    init (false) {}
  void lookup (cbhent cb);
private:
  void name_cb (ptr<hostent> h, int e);
  str hostname;
  ptr<hostent> he;
  time_t expires;
  bool resolving;
  int ttl;
  int err;
  dnsreq_t *dnsp;
  bool init;

  vec<cbhent> cbq;
};

//-----------------------------------------------------------------------

class okwc_dnscache_t {
public:
  okwc_dnscache_t () {}
  void lookup (const str &n, cbhent cb);
private:
  qhash<str, ptr<okwc_dnscache_entry_t> > cache;
};

//-----------------------------------------------------------------------

class okwc_cookie_set_t : public vec<cgi_t *> 
{
public:
  okwc_cookie_set_t () : abuf (NULL) {}
  okwc_cookie_set_t (abuf_t *a, ptr<ok::scratch_handle_t> s)
    : abuf (a), _scratch (s) {}

  void reset () { while (size ()) delete pop_back (); }

  ~okwc_cookie_set_t () { reset (); }

  cgi_t *push_back_new () 
  { return push_back (New cgi_t (abuf, true, _scratch)); }

private:
  abuf_t *abuf;
  ptr<ok::scratch_handle_t> _scratch;
};

//-----------------------------------------------------------------------

class okwc_http_hdr_t : public http_hdr_t, public pairtab_t<> {
public:

  typedef enum { OKWC_HDR_START = 1,
		 OKWC_HDR_SPC1 = 2,
		 OKWC_HDR_STATUS_NUM = 3,
		 OKWC_HDR_SPC2 = 4,
		 OKWC_HDR_STATUS_DESC = 5,
		 OKWC_HDR_EOL1 = 6,
		 OKWC_HDR_KEY = 7,
		 OKWC_HDR_SPC3 = 8,
		 OKWC_HDR_VALUE = 9,
		 OKWC_HDR_EOL2A = 10,
		 OKWC_HDR_EOL2B = 11 } state_t;
  
  okwc_http_hdr_t (abuf_t *a, okwc_cookie_set_t *ck, 
		   ptr<ok::scratch_handle_t> s)
    : async_parser_t (a), 
      http_hdr_t (a, s),
      cookie (ck), 
      state (OKWC_HDR_START), 
      contlen (0), 
      _has_body (false),
      _status_parse (HTTP_NO_STATUS),
      _status_header (HTTP_NO_STATUS),
      _clean_eoh (false),
      noins (false),
      _conn (HTTP_CONN_CLOSED) {}

  int get_contlen () const { return contlen; }
  bool is_chunked () const ;
  http_conn_mode_t connection () const { return _conn; }
  bool has_body () const { return _has_body; }

protected:
  void parse_guts ();
  void fixup ();
  bool is_set_cookie () const 
  { return (key && key.len () == 10 && mystrlcmp (key, "set-cookie")); }
  void ext_parse_cb (int status);

private:
  okwc_cookie_set_t *cookie;
  state_t state, ret_state;
  int contlen;
  bool _has_body;
  str vers, status_desc;
  int _status_parse, _status_header;
  bool _clean_eoh;
  bool noins;
  bool chunked;

  str key, val;
  http_conn_mode_t _conn;
};

//-----------------------------------------------------------------------

struct okwc_resp_t {
  okwc_resp_t (abuf_t *a, ptr<ok::scratch_handle_t> s, 
	       okwc_cookie_set_t *incook)
    : status (HTTP_OK), 
      cookies (a, s),
      _hdr (New okwc_http_hdr_t (a, incook ? incook : &cookies, s)) {}

  ~okwc_resp_t () { if (_hdr) delete _hdr; }

  okwc_resp_t (int s) : status (s), _hdr (NULL) {}

  static ptr<okwc_resp_t> alloc (abuf_t *a, ptr<ok::scratch_handle_t> s,
				 okwc_cookie_set_t *incook)
  { return New refcounted<okwc_resp_t> (a, s, incook); }

  static ptr<okwc_resp_t> alloc (int s) 
  { return New refcounted<okwc_resp_t> (s); }

  okwc_http_hdr_t *hdr () { return _hdr; }
  ptr<ok::scratch_handle_t> scratch () { return _scratch; }

  str body;
  int status;
  okwc_cookie_set_t cookies;
  okwc_http_hdr_t *_hdr;
  ptr<ok::scratch_handle_t> _scratch;
};

//-----------------------------------------------------------------------

typedef callback<void, ptr<okwc_resp_t> >::ref okwc_cb_t;

//-----------------------------------------------------------------------

//
// okwc_chunker_t
//
//   reads chunked HTTP bodies
//
class okwc_http_t;
class okwc_chunker_t : public http_hdr_t {
private:
  typedef enum { FINISH_PREV = 0,
		 START  = 1,
		 SPC1 = 2,
		 EOL1 = 3,
		 EOL2 = 4,
		 DONE = 5 } state_t;
public: 
  okwc_chunker_t (abuf_t *a, ptr<ok::scratch_handle_t> s)
    : async_parser_t (a), http_hdr_t (a, s), sz (0), state (START) {}
  ~okwc_chunker_t() { cancel(); }
  size_t get_sz () const { return sz; }
  void next_chunk () { sz = 0; state = FINISH_PREV; }
protected:
  void parse_guts ();
private:
  size_t sz;
  state_t state;
};

class okwc_http_t {
public:
  typedef enum { OKWC_HTTP_NONE = 0,
		 OKWC_HTTP_REQ = 1,
		 OKWC_HTTP_HDR = 2,
		 OKWC_HTTP_BODY = 3 } state_t;
		 
  okwc_http_t (ptr<ahttpcon> xx, const str &f, const str &h, int port,
	       int v, cgi_t *ock, okwc_cookie_set_t *incook = NULL,
	       const str &post = NULL, const str &type = NULL,
	       bool proxy_mode = false, 
	       const str &proxy_hdr_hostname = NULL);

  virtual ~okwc_http_t () { if (chunker) delete (chunker); }

  void make_req ();
  void cancel ();
  okwc_http_hdr_t *hdr () { return resp->hdr (); }

protected:
  void parse (ptr<bool> cf);
  void hdr_parsed (int status);

  void body_parse ();
  void body_parse_continue ();
  void body_chunk_finish ();
  void parsed_chunk_hdr (int status);
  void start_chunker ();
  void finish (int status);

  virtual void finish2 (int status) = 0;    // finish a body prase
  virtual void cancel2 () = 0;              // cancel current body parse
  virtual void eat_chunk (size_t s) = 0;    // process a chunk of body
  virtual void finished_meal () = 0;        // after done eating chunk

  ptr<ahttpcon> x;
  str filename;
  str hostname;
  int port;
  abuf_t abuf;

  ptr<ok::scratch_handle_t> _scratch;
  ptr<okwc_resp_t> resp;
  int vers;
  cgi_t *outcook; // cookie sending out to the server

  strbuf reqbuf;
  state_t state;
  ptr<bool> cancel_flag;
  okwc_chunker_t *chunker;
  const str _post, _type;

  bool _proxy_mode;
  str _proxy_hdr_hostname;
};

//-----------------------------------------------------------------------

//
// returns the body of the HTTP response as a big string, malloced
// in a big buffer. this is the simplest, but is wasteful
// of memory if the response is going to be parsed line-by-line.
//
class okwc_http_bigstr_t : public okwc_http_t {
public:
  okwc_http_bigstr_t (ptr<ahttpcon> xx, const str &f, 
		      const str &h, int port, okwc_cb_t c, int v, cgi_t *ock,
		      const str &post, const str &type,
		      bool proxy_mode, const str &proxy_hdr_hostname);

protected:
  void finish2 (int status);
  void cancel2 ();
  void eat_chunk (size_t s);
  void ate_chunk (str s);
  void finished_meal ();

private:
  async_dumper_t body;
  vec<str> chunks;
  okwc_cb_t okwc_cb;
};

//-----------------------------------------------------------------------

class okwc_req_t {
public:
  okwc_req_t (const str &ht, u_int16_t p, const str &fn,
	      int vers, int timeout, cgi_t *ock,
	      const str &post, const str &type);
  virtual ~okwc_req_t ();

  void launch ();
  void cancel (int status, bool from_dcb = false);

  void set_proxy_mode (const str &hostname);

protected:
  virtual okwc_http_t *okwc_http_alloc () = 0;
  void dns_cb (ptr<hostent> he, int err);

  void tcpcon_cb (int fd);
  virtual void req_fail (int status) = 0;

  const str hostname;
  const u_int16_t port;
  const str filename;
  int vers;         // 0=>HTTP/1.0;  1=>HTTP/1.1
  int timeout;
  cgi_t *outcookie; // cookies client is sending to server

  // if running in proxy mode, supply this hostname in HTTP headers
  bool _proxy_mode;
  str _proxy_hdr_hostname;
                          

  tcpconnect_t *tcpcon;
  bool waiting_for_dns;

  int fd;
  ptr<ahttpcon> x;
  okwc_http_t *http;
  timecb_t *timer;
  
  const str _post;
  const str _type;
};

//-----------------------------------------------------------------------

class okwc_req_bigstr_t : public okwc_req_t {
public:
  okwc_req_bigstr_t (const str &ht, u_int16_t p, const str &fn,
		     okwc_cb_t cb, int vers, int timeout, cgi_t *ock,
		     const str &post, const str &type)
    : okwc_req_t (ht, p, fn, vers, timeout, ock, post, type), okwc_cb (cb) {}
protected:
  virtual void req_fail (int status);
  void http_cb (ptr<okwc_resp_t> res);
  okwc_http_t *okwc_http_alloc () ;
  okwc_cb_t okwc_cb;
};

//-----------------------------------------------------------------------

okwc_req_t *
okwc_request (const str &h, u_int16_t port, const str &fn, 
	      okwc_cb_t cb, int vers = 0, int timeout = -1, 
	      cgi_t *outcook = NULL, const str &post = NULL,
	      const str &type = NULL);


//-----------------------------------------------------------------------

/*
 * wget a remote URL, but using a proxied interface.
 *
 * @param proxy_hostname the hostname of the proxy
 * @praram proxy_port the port of the proxy.
 * @param url the URL to fetch, in the form http://www.foo.com:500/xxx/yyy
 * @param cb callback to call when completed.
 * @param vers HTTP version; 1 = 1.1 and 0 = 1.0
 * @param timeout Timeout after this many seconds (or -1 if no timeout)
 * @param outcook Cookies to send to the server
 * @param post a POST payload to ship to the server.
 * @param type the Content-type: to send to the server.
 *
 * @returns an okwc_req_t, which you can later call okwc_cancel() on
 * you wish.
 */
okwc_req_t *
okwc_request_proxied (const str &proxy_hostname,
		      u_int16_t proxy_port,
		      const str &url,
		      okwc_cb_t cb,
		      int vers = 0,
		      int timeout = -1,
		      cgi_t *outcook = NULL,
		      const str &post = NULL,
		      const str &type = NULL);


//-----------------------------------------------------------------------

void
okwc_cancel (okwc_req_t *req);

//-----------------------------------------------------------------------


#endif
