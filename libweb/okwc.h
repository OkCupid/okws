
// -*-c++-*-
/* $Id$ */

#ifndef _LIBWEB_OKWC_H
#define _LIBWEB_OKWC_H

#include "cgi.h"
#include "abuf.h"
#include "aparse.h"
#include "hdr.h"
#include "str.h"
#include "web_prot.h"
#include "async.h"
#include <time.h>
#include "httpconst.h"
#include "async.h"

//
// okwc = OK Web Client
//

class okwc_cookie_set_t : public vec<cgi_t *> 
{
public:
  okwc_cookie_set_t () : abuf (NULL), bflen (0), buf (NULL) {}
  okwc_cookie_set_t (abuf_t *a, size_t l, char *b)
    : abuf (a), bflen (l), buf (b) {}

  ~okwc_cookie_set_t ()
  {
    while (size ()) delete pop_back ();
  }

  cgi_t *push_back_new () 
  { return push_back (New cgi_t (abuf, true, bflen, buf)); }

private:
  abuf_t *abuf;
  size_t bflen;
  char *buf;
};


struct okwc_resp_t {
  okwc_resp_t (abuf_t *a, size_t bfln, char *b) 
    : status (HTTP_OK), cookies (a, bfln, b) {}

  okwc_resp_t (int s) : status (s) {}

  static ptr<okwc_resp_t> alloc (abuf_t *a, size_t bfln, char *b)
  { return New refcounted<okwc_resp_t> (a, bfln, b); }

  static ptr<okwc_resp_t> alloc (int s) 
  { return New refcounted<okwc_resp_t> (s); }

  str body;
  int status;
  okwc_cookie_set_t cookies;
};

typedef callback<void, ptr<okwc_resp_t> >::ref okwc_cb_t;

#define OKWC_SCRATCH_SZ 4096

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
  
  okwc_http_hdr_t (abuf_t *a, okwc_cookie_set_t *ck, size_t bflen, char *b)
    : async_parser_t (a), http_hdr_t (a, bflen, b),
      cookie (ck), state (OKWC_HDR_START), status (HTTP_BAD_REQUEST),
      noins (false) {}

  int get_contlen () const { return contlen; }

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
  str vers, status_desc;
  int status;
  bool noins;

  str key, val;
  
};

//
// application level HTTP object
//
class okwc_http_t {
public:
  typedef enum { OKWC_HTTP_NONE = 0,
		 OKWC_HTTP_REQ = 1,
		 OKWC_HTTP_HDR = 2,
		 OKWC_HTTP_BODY = 3 } state_t;

  okwc_http_t (ptr<ahttpcon> xx, const str &f, okwc_cb_t c,
	       cgi_t *ock);

  void make_req ();
  void cancel ();

protected:
  void parse (ptr<bool> cf);
  void hdr_parsed (int status);
  void body_parsed (str bod);
  void finish (int status);

private:
  ptr<ahttpcon> x;
  str filename;
  abuf_t abuf;
  char scratch[OKWC_SCRATCH_SZ];

  ptr<okwc_resp_t> resp;
  okwc_http_hdr_t hdr;
  async_dumper_t body;
  cgi_t *outcook; // cookie sending out to the server

  okwc_cb_t okwc_cb;
  strbuf reqbuf;
  state_t state;
  ptr<bool> cancel_flag;
};

class okwc_req_t {
public:
  okwc_req_t (const str &h, u_int16_t p, const str &fn,
	      okwc_cb_t cb, int timeout, cgi_t *ock);
  ~okwc_req_t ();

  void launch ();
  void cancel (int status);

private:
  void tcpcon_cb (int fd);
  void req_fail (int status);
  void http_cb (ptr<okwc_resp_t> res);

  const str hostname;
  const u_int16_t port;
  const str filename;
  okwc_cb_t okwc_cb;
  int timeout;
  cgi_t *outcookie; // cookies client is sending to server

  tcpconnect_t *tcpcon;

  int fd;
  ptr<ahttpcon> x;
  okwc_http_t *http;
  timecb_t *timer;
};

okwc_req_t *
okwc_request (const str &h, u_int16_t port, const str &fn, 
	      okwc_cb_t cb, int timeout = -1, cgi_t *outcook = NULL );

void
okwc_cancel (okwc_req_t *req);
		

#endif
