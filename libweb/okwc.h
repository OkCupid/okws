
// -*-c++-*-
/* $Id$ */

#ifndef _LIBWEB_OKWC_H
#define _LIBWEB_OKWC_H

#include "cgi.h"
#include "abug.h"
#include "aparse.h"
#include "hdr.h"
#include "str.h"
#include "web_prot.h"
#include "async.h"
#include <time.h>

//
// okwc = OK Web Client
//

class okwc_cookie_set_t : public vec<cgi_t *> 
{
public:
  okwc_cookie_set_t (abuf_t *a, size_t l, char *b)
    : abuf (a), bflen (l), buf (b) {}

  ~okwc_cookie_set_t ()
  {
    cgi_t *c;
    while ((c = pop_back ())) delete c;
  }

  cgi_t *push_back_new () 
  { return push_back (New cgi_t (abuf, true, bflen, buf)); }

private:
  abuf_t *abuf;
  size_t buflen;
  char *buf;
};


struct okwc_resp_t {
  okwc_resp_t (abuf_t *a, size_t bfln, char *b) 
    : status (HTTP_OK),
      cookies (abuf, buflen, b) {}

  static ptr<okwd_resp_t> alloc (abuf_t *a, size_t bfln, char *b)
  { return New refcounted<okwc_resp_t> (a, bfln, b); }

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
		 OWKC_HDR_EOL2B = 11 } state_t;
  
  okwc_http_hdr_t (abuf_t *a, okwc_cookie_set_t *ck, size_t bflen, char *b)
    : async_parser_t (a), http_hdr_t (a, bflen, b),
      cookie (ck), state (OKWC_HDR_START), status (HTTP_BAD_REQUEST),
      noins (false) {}

protected:
  void parse_guts ();
  void fixup ();
  bool is_set_cookie () const 
  { return (key && key.len () == 10 && mystrlcmp (key, "set-cookie")); }
  void ext_parse_cb (int status);

private:
  okwc_cookie_set_t *ck;
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
  okwc_http_t (ptr<ahttpcon> xx, const str &f, okwc_cb_t c);

  void make_req ();

protected:
  void parse ();
  void hdr_parsed (int status);
  void body_parsed (str bod);
  void finish (int status);
  void cancel ();

private:
  ptr<ahttpcon> x;
  str filename;
  abuf_t abuf;
  char scratch[OKWC_SCRATCH_SZ];

  ptr<okwc_resp_t> resp;
  okwc_http_hdr_t hdr;
  async_dumper_t body;

  okwc_cb_t okwc_cb;
};

class okwc_req_t {
public:
  okwc_req_t (const str &h, u_int16_t p, const str &fn, cgi_t *incook,
	      okwc_cb_t cb, int timeout);
  ~okwc_req_t ();

  void launch ();

private:
  void tcpcon_cb (int fd);
  void timeout_cb ();
  void finish ();

  const str hostname;
  const u_int16_t port;
  const str filename;
  cgi_t *incookie;
  okwc_cb_t okwc_cb;
  int timeout;

  tcpconnect_t *tcpcon;

  int fd;
  ptr<ahttpcon> x;
  okwc_http_t *http;
  timecb_t *timer;
};

okwc_req_t *
okwc_request (const str &h, int port, const str &fn, cgi_t *incook,
	      okwc_cb_t cb, int timeout);
void
okwc_cancel (okwc_req_t *req);
		

#endif
