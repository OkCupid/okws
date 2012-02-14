// -*-c++-*-

/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "tame.h"
#include "vec.h"
#include "tame_io.h"

static rxx hostport ("([^:]+)(:(\\d+))?");

//-----------------------------------------------------------------------

class client_t {
public:

  client_t (str h, int p, str r, size_t ps, time_t d)
    : host (h), 
      port (p), 
      req (r), 
      packet_sz (ps), 
      delay_msec (d), 
      fd (-1) {}

  ~client_t () {}
  void run (CLOSURE);

private:
  void make_req (evi_t ev, CLOSURE);
  void read_resp (evi_t ev, CLOSURE);

  str host;
  int port;
  str req;
  size_t packet_sz;
  time_t delay_msec;
  int fd;
};

//-----------------------------------------------------------------------

tamed void 
client_t::make_req (evi_t ev)
{
  tvars {
    strbuf b;
    suio *uio;
    int rc (0);
    int trc;
    char *buf (NULL);
    size_t bytes;
  }

  b << req;
  uio = b.tosuio ();
  if (packet_sz) {
    buf = New char[packet_sz + 1];
  }

  while (uio->resid ()) {
    twait { tame::waitwrite (fd, mkevent ()); }
    if (packet_sz) {
      bytes = uio->copyout (buf, packet_sz);
      trc = write (fd, buf, bytes);
      if (trc > 0) {
	uio->rembytes (trc);
      }
    } else {
      trc = uio->output (fd);
    }
    if (trc < 0 && errno != EAGAIN) {
      warn ("write failure: %m\n");
      rc = trc;
    } else if (delay_msec) {
      twait { delaycb (0, delay_msec, mkevent ());}
    }
  }
  if (!buf) { delete [] buf; }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

#define BUFSZ 4196

tamed void 
client_t::read_resp (evi_t ev)
{
  tvars {
    char buf[BUFSZ];
    int rc (0), wrc (0);
    int outfd (1);
  }
  make_sync (outfd);

  while (fd >= 0 && rc == 0) {
    twait { tame::waitread (fd, mkevent ()); }
    rc = read (fd, buf, BUFSZ - 1);
    if (rc == 0) {
      close (fd);
      fd = -1;
    } else if (rc > 0) {
      buf[rc] = 0;
      wrc = write (outfd, buf, rc);
      if (wrc < 0) {
          warn ("read_resp: write error: %m\n");
      }
      if (wrc < rc) {
          warn ("read_resp: wrote only %d of %d bytes\n", wrc, rc);
      }
    } else if (errno != EAGAIN) {
      warn ("read error: %m\n");
      rc = -1;
    }
  }
  ev->trigger (rc);
}
#undef BUFSZ

//-----------------------------------------------------------------------

tamed void
client_t::run ()
{
  tvars {
    int rc (0);
  }
  twait { tcpconnect (host, port, mkevent (fd)); }
  if (fd < 0) {
    warn << "Cannot connect to " << host << ":" << port << "\n";
    rc = -1;
  } else {
    make_async (fd);
    twait { make_req (mkevent (rc)); }
  }
  if (rc == 0) { twait { read_resp (mkevent (rc)); } }
  exit (rc);
}

//-----------------------------------------------------------------------

static void
usage ()
{
  fatal << "usage: hcli [-p<packet-sz>] [-d<delay>] host[:port] infile\n";
}

//-----------------------------------------------------------------------

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  size_t packet_sz = 0;
  time_t delay = 0;

  int ch;
  while ((ch = getopt (argc, argv, "p:d:")) != -1) {
    switch (ch) {
    case 'd':
      if (!convertint (optarg, &delay)) {
	warn << "Cannot parse desired delay: " << optarg <<"\n";
	usage ();
      }
      break;
    case 'p':
      if (!convertint (optarg, &packet_sz)) {
	warn << "Cannot parse desired packet size: " << optarg <<"\n";
	usage ();
      }
      break;
    default:
      usage ();
    }
  }
  argc -= optind;
  argv += optind;


  if (argc != 2) 
    usage ();
  str in = file2str (argv[1]);
  if (!in)
    fatal << "Cannot open file: " << argv[1] << "\n";

  if (!hostport.match (argv[0])) 
    usage ();
  str host = hostport[1];
  str port_s = hostport[3];
  int port (80);
  if (port_s) {
    if (!convertint (port_s, &port)) usage ();
  }

  ptr<client_t> c = New refcounted<client_t> (host, port, in, packet_sz, delay);
  c->run ();
  amain ();
}

//-----------------------------------------------------------------------

