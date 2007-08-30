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
#include "okwc3.h"

static rxx url_rxx ("http://([^:/]+)(:(\\d+))?(/(.*))?");

void
usage ()
{
  warn << "usage: [http_proxy=http://<server>:<port>] okwc <url> <post>\n";
  exit (1);
}

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    str post;
    str typ;
    str hostname;
    u_int16_t port (80);
    str port_str;
    str filename;
    ptr<okwc3::agent_get_t> cli;
    ptr<okwc3::resp_simple_t> resp;
    int status;
    str prx;
    str connect_to;
  }

  okwc_def_contlen *= 10;
  if (argc != 2 && argc != 3) 
    usage ();

  prx = getenv ("http_proxy");
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

  hostname = url_rxx[1];
  port = 80;
  port_str = url_rxx[3];
  if (port_str && port_str.len ()) 
    assert (convertint (port_str, &port));

  if (prx) {
    filename = argv[1];
    cli = New refcounted<okwc3::agent_get_proxied_t> (hostname, port);
  } else {
    filename = url_rxx[5];
    cli = New refcounted<okwc3::agent_get_direct_t> (hostname, port);
  }

  twait { cli->get (filename, mkevent (status, resp), 1); }

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
