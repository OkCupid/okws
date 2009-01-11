// -*-c++-*-
/* $Id$ */

#ifndef __LIBAOK_OK3_H__
#define __LIBAOK_OK3_H__

#include "ok.h"

/*
 * okclnt3_t: like okclnt_t and okclnt2_t, a class that corresponds to
 * an incoming HTTP client request.  okclnt3_t is difference since it
 * supports HTTP/1.1 pipelining, and therefore can accept multiple
 * requests per one connection.
 */
class okclnt3_t {
public:

  //------------------------------------------------------------------------

  class req_t : http_parser_cgi_t {
  public:

  };

  //------------------------------------------------------------------------

  class resp_t {

  };

  //------------------------------------------------------------------------

  virtual void process (ptr<req_t> req, ptr<resp_t> resp, evi_t ev) = 0;

  //------------------------------------------------------------------------

private:
  ptr<ahttpcon> _x;

};



#endif /* __LIBAOK_OK3_H__ */
