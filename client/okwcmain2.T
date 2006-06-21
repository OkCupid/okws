// -*-c++-*-

/* $Id: okwcmain.C 1635 2006-03-16 17:28:10Z max $ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "okwc.h"
#include "aios.h"
#include "parseopt.h"
#include "rxx.h"
#include "tame.h"
#include "okwc2.h"

static rxx url_rxx ("http://([^:/]+)(:(\\d+)/)?(.*)");

void
usage ()
{
  warn << "usage: okwc <url> <post>\n";
  exit (1);
}

TAMED static void
main2 (int argc, char **argv)
{
  VARS {
    str post;
    str typ;
    str hostname;
    u_int16_t port (80);
    str port_str;
    str filename;
    ptr<okwc2_simple_t> cli;
    ptr<okwc2_resp_simple_t> resp;
    int status;
  }

  okwc_def_contlen *= 10;
  if (argc != 2 && argc != 3) 
    usage ();
  if (!url_rxx.match (argv[1])) 
    usage ();

  if (argc == 3) {
    post = argv[2];
    typ = "application/x-www-form-urlencoded";
  }

  hostname = url_rxx[1];
  port = 80;
  port_str = url_rxx[3];
  if (port_str && port_str.len ()) 
    assert (convertint (port_str, &port));
  filename = url_rxx[4];

  cli = New refcounted<okwc2_simple_t> (hostname, port);
  resp = New refcounted<okwc2_resp_simple_t> ();
  
  // 10 second timeout
  BLOCK { cli->req (filename, @(status, resp), 10); }

  aout << "status: " << status << "\n";
  if (status == HTTP_MOVEDTEMP || status == HTTP_MOVEDPERM) {
    aout << "Redirect to: " << (* resp->hdr () )["location"] << "\n";
  } else if (resp->body ())
    aout << "\nbody: " << resp->body () << "\n";
  exit (0);
}


int 
main (int argc, char *argv [])
{
  main2 (argc, argv);
  amain ();
}
