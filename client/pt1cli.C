#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "stdlib.h"
#include "time.h"

static rxx hostport ("([^:]+)(:(\\d+))?");
int nreq;
int nconcur;
int nrunning;
bool sdflag;
bool seda;

class hclient_t {
public:
  hclient_t (str h, int p, str r, int i) 
    : host (h), port (p), req (r), id (i), bp (buf), reqsz (-1), body (NULL) {}
  ~hclient_t () 
  {
    if (fd >= 0) {
      fdcb (fd, selread, NULL);
      // close (fd);
      tcp_abort (fd);
    }
  }
  void run ();

private:
  void connected (int f);
  void canread ();
  void writewait ();
  void cexit (int rc);

  char buf[4096];
  str host;
  int port;
  str req;
  int fd;
  int id;
  char *bp;
  int reqsz;
  char *body;
  suio uio;
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

static rxx sz_rxx ("[C|c]ontent-[l|L]ength: (\\d+)\\r\\n");
void
hclient_t::canread ()
{
  int rc = uio.input (fd);
  if (rc == 0) {
    cexit (0);
    return;
  }
  if (!seda) {
    if (rc == -1 && errno != EAGAIN) {
      warn ("read error: %m\n");
      cexit (-1);
    }
    while (uio.resid ()) 
      uio.output (1);
  } else {
    rc = uio.copyout (bp);
    bp += rc;
    if (reqsz < 0) {
      if (sz_rxx.search (buf)) {
	if (!convertint (sz_rxx[1], &reqsz)) {
	  warn ("invalid length: %s\n", sz_rxx[1].cstr ());
	  cexit (-1);
	}
      }
    }
    if (!body) 
      body = strstr (buf, "\r\n\r\n");
    if (body && (bp - body >= reqsz)) {
      write (1, buf, bp - buf);
      warn ("ok, closing up!\n");
      cexit (0);
    }
  }
}

void 
hclient_t::connected (int f)
{
  fd = f;
  strbuf b (req);
  int id = random() % 30000;
  b << id <<" HTTP/1.0\r\nConnection: close\r\n\r\n";
  if (fd < 0) {
    if (seda) {
      warn << "cannot connect to host\n";
      cexit (-1);
      return;
    } else
      fatal << "cannot connect to host\n";
  }
  suio *uio = b.tosuio ();
  while (uio->resid ()) {
    writewait ();
    if (uio->output (fd) < 0)
      fatal << "write error\n";
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
  warn << "connecting to: " << host << "\n";
  tcpconnect (host, port, wrap (this, &hclient_t::connected));
}

static void
usage ()
{
  fatal << "usage: ptcli host[:port] infile <nreq>\n";
}

int 
main (int argc, char *argv[])
{
  seda = false;
  srandom(time(0));
  setprogname (argv[0]);
  if (argc < 3 || argc > 5)
    usage ();

  int n = 1;
  nconcur = 500; 
  if (argc >= 4 && !convertint (argv[3], &n)) 
    usage ();
  if (argc == 5 && !convertint (argv[4], &nconcur)) 
    usage ();
    
  //str in = file2str (argv[2]);
  str in;
  if (argv[2][0] == 'p') 
    in = "GET /test.php?id=";
  else if (argv[2][0] == 's') {
    seda = true;
    in = "GET /test?x=";
  } else
    in = "GET /pt1?id=";
 
  if (!in)
    fatal << "Cannot open file: " << argv[2] << "\n";
  if (!hostport.match (argv[1])) 
    usage ();
  str host = hostport[1];
  str port_s = hostport[3];
  int port;
  if (port_s) {
    if (!convertint (port_s, &port)) usage ();
  } else {
    port = 80;
  }
  nrunning = 0;
  nreq = n;
  sdflag = true;
  for (int i = 0; i < n; i++) {
    hclient_t *h = New hclient_t (host, port, in, i);
    h->run ();
  }
  amain ();
}
