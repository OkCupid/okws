
// -*-c++-*-
/* $Id$ */

#ifndef _LIBWEB_OKWC_H
#define _LIBWEB_OKWC_H

#include "str.h"
#include "web_prot.h"
#include "async.h"
#include <time.h>

//
// okwc = OK Web Client

struct okwc_resp_t {
  okwc_resp_t (int s) : status (s) {}

  static ptr<okwd_resp_t> alloc (int s) 
  { return New refcounted<okwc_resp_t> (s); }

  str body;
  int status;
  cgi_t cookie;
};

typedef callback<void, ptr<okwc_resp_t> >::ref okwc_cb_t;

#define OKWC_SCRATCH_SZ 4096

//
// application level HTTP object
//
struct okwc_http_t : public virtual async_parser_t {
  okwc_http_t (ptr<ahttpcon> xx, const str &f, int to, okwc_cb_t c);

  void make_req ();
  void cancel ();

  ptr<ahttpcon> x;
  str filename;
  abuf_t abuf;

  okwc_http_hdr_t hdr;
  okwc_http_body_t body;
  int timeout;

  char scratch[OKWC_SCRATCH_SZ];
  ptr<okwc_resp_t> resp;
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
