
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
http_parser_cgi_t::finish2 (int s1, int s2)
{
  if (s1 != HTTP_OK)
    finish (s1);
  else if (s2 != HTTP_OK)
    finish (s2);
  else
    finish (HTTP_OK);
}

void
http_parser_cgi_t::v_parse_cb1 (int status)
{
  if (hdr.mthd == HTTP_MTHD_POST) {
    cgi = &post;

    if (hdr.contlen > int (ok_reqsize_limit)) {
      finish (HTTP_NOT_ALLOWED);
      return;
    } else if (hdr.contlen < 0)
      hdr.contlen = ok_reqsize_limit -1;

    abuf.setlim (hdr.contlen);

    cbi::ptr pcb = wrap (this, &http_parser_cgi_t::finish2, status);
    if (!mpfd_parse (pcb)) 
      post.parse (pcb);
  } else if (hdr.mthd == HTTP_MTHD_GET) {
    cgi = &url;
    finish (status);
  } else {
    finish (HTTP_NOT_ALLOWED);
  }
}

bool
http_parser_cgi_t::mpfd_parse (cbi::ptr pcb)
{
  str boundary;
  if (cgi_mpfd_t::match (hdr, &boundary)) {
    if (mpfd_flag) {
      mpfd = cgi_mpfd_t::alloc (&abuf, hdr.contlen, boundary);
      cgi = mpfd;
      mpfd->parse (pcb);
    } else {
      warn << "file upload attempted when not permitted\n";
    }
    return true;
  }
  return false;
}

void
http_parser_base_t::clnt_timeout ()
{
  tocb = NULL;
  v_cancel ();
  stop_abuf ();
  (*cb) (HTTP_TIMEOUT);
}



