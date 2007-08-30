
/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "okwc.h"
#include "aios.h"
#include "parseopt.h"
#include "rxx.h"


static rxx url_rxx ("http://([^:/]+)(:(\\d+))?(/(.*))?");

void
usage ()
{
  warn << "usage: okwc <url> <post>\n";
  exit (1);
}

void
reqcb (ptr<okwc_resp_t> res)
{
  aout << "status: " << res->status << "\n";
  if (res->status == HTTP_MOVEDTEMP || res->status == HTTP_MOVEDPERM) {
    aout << "Redirect to: " << (* res->hdr () )["location"] << "\n";
  } else if (res->body)
    aout << "\nbody: " << res->body << "\n";
  exit (0);
}

int 
main (int argc, char *argv [])
{
  okwc_def_contlen *= 10;
  str post;
  str typ;
  if (argc != 2 && argc != 3) 
    usage ();

  str connect_to;
  str prx = getenv ("http_proxy");
  str filename;
  if (prx) {
    connect_to = prx;
  } else {
    connect_to = argv[1];
  }


  if (!url_rxx.match (connect_to)) 
    usage ();

  if (argc == 3) {
    post = argv[2];
    typ = "application/x-www-form-urlencoded";
  }

  str hostname = url_rxx[1];
  u_int16_t port = 80;
  str port_str = url_rxx[3];
  if (port_str && port_str.len ())  {
    bool rc = convertint (port_str, &port);
    assert (rc);
  }

  if (prx) {
    filename = argv[1];
    okwc_request_proxied (hostname, port, filename, wrap (reqcb), 0, 100,
			  NULL, post, typ);
  } else {
    filename = url_rxx[5];
    okwc_request (hostname, port, filename, wrap (reqcb), 0, 100,
		  NULL, post, typ);
  }

  amain ();
}

