
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_AHPARSE_H
#define _LIBAHTTP_AHPARSE_H

#include "httpconst.h"
#include "arpc.h"
#include "ahttp.h"
#include "cgi.h"
#include "resp.h"
#include "pub.h"
#include "inhdr.h"
#include "pslave.h"
#include "pubutil.h"
#include "mpfd.h"

#define HTTP_PARSE_BUFLEN 0x4000
#define HTTP_PARSE_BUFLEN2 0x1000

class http_parser_base_t {
public:
  http_parser_base_t (ptr<ahttpcon> xx, u_int to) 
    : x (xx), abuf (New abuf_con_t (xx), true),
      timeout (to ? to : ok_clnt_timeout),
      buflen (HTTP_PARSE_BUFLEN), tocb (NULL),
      destroyed (New refcounted<bool> (false)) {}
  virtual ~http_parser_base_t ();

  str operator[] (const str &k) const { return hdr_cr ().lookup (k); }
  virtual http_inhdr_t *hdr_p () = 0;
  virtual const http_inhdr_t &hdr_cr () const = 0;
  ptr<ahttpcon> get_x () const { return x; }

  void parse (cbi cb);
protected:
  virtual void v_parse_cb1 (int status) { finish (status); }
  virtual void v_cancel () {}

  void parse_cb1 (int status);
  void finish (int status);
  void clnt_timeout ();
  void stop_abuf ();

  ptr<ahttpcon> x;
  abuf_t abuf;
  u_int timeout;
  size_t buflen;
  char scratch[HTTP_PARSE_BUFLEN];
  timecb_t *tocb;
  cbi::ptr cb;
  ptr<bool> destroyed;
};

class http_parser_raw_t : public http_parser_base_t {
public:
  http_parser_raw_t (ptr<ahttpcon> xx, u_int to = 0)
    : http_parser_base_t (xx, to), 
      hdr (&abuf, NULL, NULL, buflen, scratch) {}

  http_inhdr_t *hdr_p () { return &hdr; }
  const http_inhdr_t &hdr_cr () const { return hdr; }
  void v_cancel () { hdr.cancel (); }

  static ptr<http_parser_raw_t> alloc (ptr<ahttpcon> xx, u_int t = 0)
  { return New refcounted<http_parser_raw_t> (xx, t); }

  http_inhdr_t hdr;
};

class http_parser_cgi_t : public http_parser_base_t {
public:
  http_parser_cgi_t (ptr<ahttpcon> xx, u_int to = 0)
    : http_parser_base_t (xx, to), buflen2 (HTTP_PARSE_BUFLEN2),
      cookie (&abuf, true, buflen2, scratch2),
      url (&abuf, false, buflen2, scratch2),
      post (&abuf, false, buflen, scratch),
      mpfd (NULL),
      mpfd_flag (false),
      hdr (&abuf, &url, &cookie, buflen, scratch) 
  {}
  ~http_parser_cgi_t () { if (mpfd) delete mpfd; }

  http_inhdr_t * hdr_p () { return &hdr; }
  const http_inhdr_t &hdr_cr () const { return hdr; }

  void v_cancel () { hdr.cancel (); post.cancel (); }
  void v_parse_cb1 (int status);
  void finish2 (int s1, int s2);
  void enable_file_upload () { mpfd_flag = true; }

  bool mpfd_parse (cbi::ptr pcb);

protected:
  size_t buflen2;

  cgi_t cookie;
  cgi_t url;
  cgi_t post;
  cgi_mpfd_t *mpfd;
  cgiw_t cgi;  // wrapper set to either url or post, depending on the method
  char scratch2[HTTP_PARSE_BUFLEN2];

private:
  bool mpfd_flag;

public:
  http_inhdr_t hdr;

};

#endif
