// -*-c++-*-

/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "tame_io.h"

static rxx hostport ("([^:]+)(:(\\d+))?");

//-----------------------------------------------------------------------

class client_t {
public:

  client_t (str host, int port, str r , size_t packet_sz, time_t d)
    : host (h), 
      port (p), 
      req (r), 
      packet_sz (psz), 
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
hclient_t::make_req (evi_t ev)
{
  tvars {
    fd = f;
    strbuf b;
    suio *uio;
    struct timeval tv;
    char *buf;
    int psz;
    int rc = 0;
    int trc;
  }

  b << req;
  uio = b.tosuio ();
  psz = packet_sz;
  if (psz == 0) { psz = -1; }

  while (uio->resid ()) {
    twait { tame::writewait (_fd, mkevent ()); }
    trc = uio->output (_fd, psz);
    if (trc < 0 && errno != EAGAIN) {
      warn ("write failure: %m\n");
      rc = trc;
    } else if (delay_msec) {
      twait { delaycb (0, delay_msec, mkevent ());}
    }
  }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void 
client_t::read_resp (evi_t ev)
{
#define BUFSZ 4196
  tvars {
    char buf[BUFSZ];
    int rc (0);
    int outfd (1);
  }
  make_sync (outfd);

  while (_fd >= 0 && rc == 0) {
    twait { tame::writeread (_fd, mkevent ()); }
    rc = read (_fd, buf, BUFSZ - 1);
    if (rc == 0) {
      close (_fd);
      _fd = -1;
    } else if (rc > 0) {
      buf[rc] = 0;
      write (outfd, buf, rc);
    } else if (errno != EAGAIN) {
      warn ("read error: %m\n");
      rc = -1;
    }
  }
  ev->trigger (rc);
#undef BUFSZ
}

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
    make_async (_fd);
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
      if (!parseint (optarg, &delay)) {
	warn << "Cannot parse desired delay: " << optarg <<"\n";
	usage ();
      }
      break;
    case 'p':
      if (!parseint (optarg, &packet_size)) {
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
  int n = 1;
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

  ptr<client_t> c = New refcounted<cilent_t> (host, port, in, packet_sz, delay);
  c->run ();
  amain ();
}

//-----------------------------------------------------------------------

