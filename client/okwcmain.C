
/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "okwc.h"
#include "aios.h"
#include "parseopt.h"
#include "rxx.h"


static rxx url_rxx ("http://([^:/]+)(:(\\d+)/)?(.*)");

void
usage ()
{
  warn << "usage: okwc <url>\n";
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
  if (argc != 2) 
    usage ();
  if (!url_rxx.match (argv[1])) 
    usage ();

  str hostname = url_rxx[1];
  u_int16_t port = 80;
  str port_str = url_rxx[3];
  if (port_str && port_str.len ()) 
    assert (convertint (port_str, &port));
  str filename = url_rxx[4];

  okwc_request (hostname, port, filename, wrap (reqcb), 1, 100);
  amain ();
}

