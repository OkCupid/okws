
// -*-c++-*-
/* $Id$ */

#ifndef _LIBWEB_WEBCLI_H
#define _LIBWEB_WEBCLI_H

#include "str.h"
#include "web_prot.h"
#include "async.h"
#include <time.h>

struct webcli_resp_t {
  webcli_resp_t () {}
  str body;
  int status;
  cgi_t cookie;
};

typedef callback<void, ptr<webcli_resp_t> > webcli_cb_t;

#define WEBCLI_SCRATCH_SZ 4096

struct webcli_t {
  webcli_t (ptr<ahttpcon> xx, const str &f, int to, cbv c);

  ptr<ahttpcon> x;
  str filename;
  abuf_t abuf;
  http_hdr_t hdr;
  int timeout;
  cbv cb;

  char scratch[WEBCLI_SCRATCH_SZ];
};

webcli_t 


#endif
