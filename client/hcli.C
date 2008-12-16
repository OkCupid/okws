
/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"


static rxx hostport ("([^:]+)(:(\\d+))?");
int nreq;
int nconcur;
int nrunning;
bool sdflag;
bool trickle;

class hclient_t {
public:
  hclient_t (str h, int p, str r, int i) 
    : host (h), port (p), req (r), id (i) {}
  ~hclient_t () 
  {
    fdcb (fd, selread, NULL);
    tcp_abort (fd);
  }
  void run ();

private:
  void connected (int f);
  void canread ();
  void writewait ();
  void cexit (int rc);

  str host;
  int port;
  str req;
  int fd;
  int id;
};

vec<hclient_t *> q;

void
hclient_t::writewait ()
{
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (fd, &fds);
  select (fd + 1, NULL, &fds, NULL, NULL);
}

void
hclient_t::cexit (int c)
{
  warn << "at cexit: " << id << "\n";
  --nrunning;
  while (nrunning < nconcur && q.size ()) {
    warn << "launched queued req: " << id << "\n";
    q.pop_front ()->run ();
  }
  if (!--nreq && sdflag)
    exit (c);
  warn << "done: " << id << " (nreq: " << nreq << ")\n";
  delete this;
  return;
}

void
hclient_t::canread ()
{
  suio uio;
  int rc = uio.input (fd);
  if (rc == 0) {
    cexit (0);
    return;
  }
  if (rc == -1 && errno != EAGAIN) {
    warn ("read error: %m\n");
    cexit (-1);
  }
  while (uio.resid ()) 
    uio.output (1);
}

void 
hclient_t::connected (int f)
{
  fd = f;
  strbuf b (req);
  req << "\r\n\r\n";
  if (fd < 0)
    fatal << "cannot connect to host\n";
  suio *uio = b.tosuio ();
  char c;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;

  while (uio->resid ()) {
    writewait ();

    if (trickle) {

      // 1 byte at a time
      uio->copyout (&c, 1);
      if (write (fd, &c, 1)  == 1)
	uio->rembytes (1);
      select (0, NULL, NULL, NULL, &tv);
      warn ("trickle: %c\n", c);

    } else {
      if (uio->output (fd) < 0)
	fatal << "write error\n";
    }
  }
  fdcb (fd, selread, wrap (this, &hclient_t::canread));
}

void
hclient_t::run ()
{
  warn << "run: " << id << "\n";
  if (nrunning >= nconcur) {
    warn << "queuing: " << id << "\n";
    q.push_back (this);
    return;
  }
  nrunning++;
  warn << "running: " << id << " (nrunning: " << nrunning << ")\n";
  tcpconnect (host, port, wrap (this, &hclient_t::connected));
}

static void
usage ()
{
  fatal << "usage: hcli host[:port] infile <nreq>\n";
}

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  trickle = false;

  int ch;
  while ((ch = getopt (argc, argv, "t")) != -1) {
    switch (ch) {
    case 't':
      trickle = true;
      break;
    default:
      usage ();
    }
  }
  argc -= optind;
  argv += optind;

   

  if (argc != 2 && argc != 3)
    usage ();

  int n = 1;
  if (argc == 3 && !convertint (argv[2], &n)) 
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
  nrunning = 0;
  nconcur = 20;
  nreq = n;
  sdflag = true;
  for (int i = 0; i < n; i++) {
    hclient_t *h = New hclient_t (host, port, in, i);
    h->run ();
  }
  amain ();
}
