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
#include "tame_connectors.h"
#include "oksslutil.h"

static rxx url_rxx ("http(s)?://([^:/]+)(:(\\d+))?(/(.*))?");

void
usage ()
{
  warn << "usage: okwc [-t <timeout>] [-p <proxy>] <url> [<post-file>]\n";
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
    int status (HTTP_TIMEOUT);
    str prx;
    str connect_to;
    int ch;
    int timeout (-1);
    bool https (false);
    vec<str> extra_headers;
  }

  okssl::init_ssl_internals ();
  okwc_def_contlen *= 10;

  while ((ch = getopt (argc, argv, "t:p:h:")) != -1) {
    switch (ch) {
    case 't':
      if (!convertint (optarg, &timeout))
	usage ();
      break;
    case 'p':
      prx = optarg;
      break;
    case 'h':
      extra_headers.push_back (optarg);
      break;
    default:
      usage ();
    }
  }

  argc -= optind;
  argv += optind;


  if (argc != 1 && argc != 2) 
    usage ();

  if (!prx)
    prx = getenv ("http_proxy");

  if (prx) {
    filename = argv[0];
    connect_to = prx;
  } else {
    connect_to = argv[0];
  }

  if (!url_rxx.match (connect_to))
    usage ();

  if (url_rxx[1] && url_rxx[1] == "s") {
    https = true;
  }

  if (argc == 2) {
    str file = argv[1];
    if (!(post = file2str (file))) {
      warn ("Cannot open post file %s: %m\n", file.cstr ());
      usage ();
    }
    typ = "application/x-www-form-urlencoded";
  }

  hostname = url_rxx[2];
  port = https ? 443 : 80;
  port_str = url_rxx[4];

  if (port_str && port_str.len ()) {
    bool rc = convertint (port_str, &port);
    assert (rc);
  }

  if (prx) {
    cli = New refcounted<okwc3::agent_get_proxied_t> (hostname, port);
  } else {
    filename = url_rxx[6];
    cli = New refcounted<okwc3::agent_get_direct_t> (hostname, port);
  }

  twait { 
    cli->get (filename, 
	      connector::timeout (mkevent (status, resp), timeout),
	      1, NULL, https, 
	      post, &extra_headers);
  }

  aout << "status: " << status << "\n";
  if (status == HTTP_MOVEDTEMP || status == HTTP_MOVEDPERM) {
    aout << "Redirect to: " << (* resp->hdr () )["location"] << "\n";
  } else if (resp && resp->body ())
    aout << "\nbody: " << resp->body () << "\n";

  exit (0);
}


int 
main (int argc, char *argv [])
{
  main2 (argc, argv);
  amain ();
}
