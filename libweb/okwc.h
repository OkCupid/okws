
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
  okwc_resp_t () {}
  str body;
  int status;
  cgi_t cookie;
};

typedef callback<void, ptr<okwc_resp_t> > okwc_cb_t;

#define OKWC_SCRATCH_SZ 4096

//
// application level HTTP object
//
struct okwc_http_t {
  okwc_http_t (ptr<ahttpcon> xx, const str &f, int to, cbv c);

  ptr<ahttpcon> x;
  str filename;
  abuf_t abuf;
  http_hdr_t hdr;
  int timeout;
  cbv cb;

  char scratch[OKWC_SCRATCH_SZ];
  ptr<okwc_resp_t> resp;
};

struct okwc_req_t {
  okwc_req_t (const str &h, int p, const str &fn, cgi_t *incook,
	      okwc_cb_t cb, int timeout);

  const str hostname;
  const int port;
  const str filename;
  cgi_t *incookie;
  okwc_cb_t okwc_cb;
  int timeout;

  tcpconnect_t *tcpchan;
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
