
#include "ahparse.h"

http_parser_base_t::~http_parser_base_t ()
{
  if (tocb) {
    timecb_remove (tocb);
    tocb = NULL;
  }
}

void
http_parser_base_t::parse (cbi c)
{
  cb = c;
  tocb = delaycb (timeout, 0, wrap (this, &http_parser_base_t::clnt_timeout));
  hdr_p ()->parse (wrap (this, &http_parser_base_t::parse_cb1));
}

void
http_parser_base_t::parse_cb1 (int status)
{
  if (status != HTTP_OK) {
    finish (status);
    return;
  }
  v_parse_cb1 (status);
}

void
http_parser_base_t::finish (int status)
{
  if (tocb) {
    timecb_remove (tocb);
    tocb = NULL;
  }
  // XXX - it's crucial not to touch anything inside the class after
  // this CB, since the cb might destroy this object
  (*cb) (status);
}

void
http_parser_cgi_t::v_parse_cb1 (int status)
{
  if (hdr.mthd == HTTP_MTHD_POST) {
    cgi = &post;
    abuf.set_ignore_finish (false);
    if (hdr.reqsize >= 0)
      abuf.setlim (hdr.reqsize);
    post.parse (wrap (static_cast<http_parser_base_t *> (this),
		      &http_parser_base_t::finish, status));
  } else if (hdr.mthd == HTTP_MTHD_GET) {
    cgi = &url;
    finish (status);
  } else {
    finish (HTTP_NOT_ALLOWED);
  }
}

void
http_parser_base_t::clnt_timeout ()
{
  tocb = NULL;
  v_cancel ();
  (*cb) (HTTP_TIMEOUT);
}
