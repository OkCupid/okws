
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "tame.h"
#include "rxx.h"
#include "okwcxml.h"
#include "okxmlobj.h"
#include "tame_connectors.h"

static void usage (void)
{
  warnx << "usage: " << progname << " [-n <ntimes>] [-d] [-t<timeout>] <url>\n";
  exit (1);
}

static rxx url_rxx ("http://([^:/]+)(:(\\d+)/)?(.*)");

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    str hostname;
    u_int16_t port (80);
    str port_str;
    str filename;
    int tmo;
    int i;
    int ch;
    bool debug (false);
    bool verbose (false);
    int times (5);
    ptr<okwc_resp_t> resp;
  }

  while ((ch = getopt (argc, argv, "n:vdt:")) != -1) {
    switch (ch) {
    case 'n':
      if (!convertint (optarg, &times))
	usage ();
      break;
    case 'd':
      debug = true;
      break;
    case 'v':
      verbose = true;
      break;
    case 't':
      if (!convertint (optarg, &tmo))
	usage ();
      break;
    default:
      usage ();
      break;
    }
  }
  
  argc -= optind;
  argv += optind;

  if (argc != 1) usage ();

  if (!url_rxx.match (argv[0]))
    usage ();


  hostname = url_rxx[1];
  port_str = url_rxx[3];
  if (port_str && port_str.len ())
    assert (convertint (port_str, &port));
  if (url_rxx[4])
    filename = url_rxx[4];
  else
    filename = "/";

  for (i = 0; i < times; i++) {
    twait {
      okwc_request (hostname, port, filename, mkevent (resp), 1, tmo);
    }
    warn << "Response: " << resp->status << "\n";
  }

  twait { delaycb (3, 0, mkevent ()); }
  exit (0);
}


int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}
