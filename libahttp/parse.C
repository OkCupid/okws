
#include "ahparse.h"

http_parser_base_t::~http_parser_base_t ()
{
  if (tocb) {
    timecb_remove (tocb);
    tocb = NULL;
  }
  *destroyed = true;
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
  // If we don't stop the abuf, we might be fooled into parsing
  // again on an EOF.
  stop_abuf ();

  // XXX -- we (stupidly) have a cyclic data structure here; that is,
  // we might have a refcounted this wrapped in the callback given.
  // this, if we haven't be destroyed, we'll need to set the CB equal 
  // to NULL.  in some cases, so doing will cause the object to be 
  // destroyed.
  ptr<bool> local_destroyed = destroyed;
  (*cb) (status);
  if (!*local_destroyed)
    cb = NULL;
}

void
http_parser_base_t::stop_abuf ()
{
  abuf.finish ();
}

void
http_parser_cgi_t::v_parse_cb1 (int status)
{
  if (hdr.mthd == HTTP_MTHD_POST) {
    cgi = &post;
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
  stop_abuf ();
  (*cb) (HTTP_TIMEOUT);
}
