
#include "resp.h"
#include "httpconst.h"

http_status_set_t http_status;

http_status_set_t::http_status_set_t ()
{
  add (100, "Continue");
  add (101, "Switching Protocols");
  add (HTTP_OK, "OK");
  add (201, "Created");
  add (202, "Accepted");
  add (203, "Non-Authoratative Information");
  add (204, "No Content");
  add (205, "Resent Content");
  add (206, "Partial Content");
  add (300, "Multiple  Choices");
  add (HTTP_REDIRECT, "Moved Permanently");
  add (302, "Found");
  add (303, "See Other");
  add (304, "Not Modified");
  add (305, "Use Proxy");
  add (307, "Temporary Redirect");
  add (HTTP_BAD_REQUEST, "Bad Request");
  add (401, "Unauthorized");
  add (402, "Payment Required");
  add (403, "Forbidden");
  add (HTTP_NOT_FOUND, "Not Found");
  add (HTTP_NOT_ALLOWED, "Method Not Allowed");
  add (406, "Not Acceptable");
  add (407, "Proxy Authentication Required");
  add (HTTP_TIMEOUT, "Request Time-out");
  add (409, "Conflict");
  add (410, "Gone");
  add (411, "Length Required");
  add (412, "Precondition Failed");
  add (413, "Request Entity Too Large");
  add (HTTP_URI_TOO_BIG, "Request-URI Too Large");
  add (415, "Unsupported Media Type");
  add (416, "Requested range not satisfied");
  add (417, "Expectation Failed");
  add (500, "Internal Server Error");
  add (HTTP_NOT_IMPLEMENTED, "Not Implemented");
  add (502, "Bad Gateway");
  add (HTTP_UNAVAILABLE, "Service Unavailable");
  add (504, "Gateway Time-out");
  add (505, "HTTP Version not supported");
  add (HTTP_CLIENT_EOF, "Client EOF before response sent.");
}

void
http_resp_header_t::fill (bool gz)
{
  add_date ();
  add ("Content-Type", "text/html");
  add_closed ();
  add ("Cache-control", "private");
  add_server ();
  if (gz) gzip ();
}

void
http_resp_header_redirect_t::fill (const str &s)
{
  add_date ();
  add_closed ();
  add_server ();
  add ("Location", s);
}

void
http_resp_header_t::fill (bool gz, int len)
{
  fill (gz);
  if (!gz) add (http_hdr_size_t (len));
}

void
http_resp_header_t::gzip ()
{
  add ("Content-Encoding", "gzip");
  add ("Transfer-Encoding", "chunked");
}

strbuf
http_resp_header_t::to_strbuf () const
{
  strbuf b;
  b << "HTTP/";
  switch (vers) {
  case 1:
    b << "1.1 ";
    break;
  default:
    b << "1.0 ";
    break;
  }
  b << status;
  if (status == HTTP_OK)
    b << " OK";
  b << "\r\n";
  int lim = fields.size ();
  for (int i = 0; i < lim; i++) {
    b << fields[i].name << ": " << fields[i].val << "\r\n";
  }
  b << "\r\n";
  return b;
}

str
http_status_set_t::get_desc (int n, str *l) const
{
  http_status_t *s = tab[n];
  if (!s) {
    if (l) *l = "";
    return "";
  }
  if (l) *l = s->ldesc;
  return s->sdesc;
}

ptr<http_pub_t>
http_pub_t::alloc (int n, const pub_t &p, const str &fn, aarr_t *env, htpv_t v)
{
  ptr<http_pub_t> ret (New refcounted<http_pub_t> (n, p, fn, env, v));
  if (ret->err) return NULL;
  else return ret;
}

u_int
http_response_t::send (ptr<ahttpcon> x, cbv::ptr cb)
{ 
  const strbuf &b = to_strbuf ();
  u_int ret = b.tosuio ()->resid ();
  x->send (b, cb); 
  return ret;
}

