#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "stdlib.h"
#include "time.h"

static rxx hostport ("([^:]+)(:(\\d+))?");
int nreq;
int nreq_fixed;
int nconcur;
int nrunning;
bool sdflag;
bool noisy;

timespec startt;

typedef enum { OKWS = 1, PHP = 2, SEDA = 3 } pt1cli_mode_t;
pt1cli_mode_t mode;

void do_exit (int rc);
int stop_timer (const timespec &t);
vec<int> latencies;

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

  timespec cli_start;
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
  int shit;
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
  if (noisy) warn << "at cexit: " << id << "\n";
  --nrunning;
  while (nrunning < nconcur && q.size ()) {
    if (noisy) warn << "launched queued req: " << id << "\n";
    q.pop_front ()->run ();
  }
  if (!--nreq && sdflag)
    do_exit (c);
  if (noisy) warn << "done: " << id << " (nreq: " << nreq << ")\n";
  latencies.push_back ((fd > 0) ? stop_timer (cli_start) : -1);
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
  if (mode != SEDA) {
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
      if (noisy) warn ("ok, closing up!\n");
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
    if (mode == SEDA) {
      if (noisy) warn << "cannot connect to host\n";
      cexit (-1);
      return;
    } else {
      warn << "cannot connect to host\n";
      do_exit (2);
    }
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
do_exit (int c)
{
  int total_tm = stop_timer (startt);
  warnx << nreq_fixed << "," << total_tm << "\n";
  u_int sz = latencies.size ();
  for (u_int i = 0; i < sz; i++) 
    warnx << latencies[i] << "\n";
  exit (c);
}

int
stop_timer (const timespec &tm)
{
  int ret = (tsnow.tv_nsec - tm.tv_nsec) / 1000;
  int tmp = (tsnow.tv_sec - tm.tv_sec) * 1000000;
  ret += tmp;
  return ret;
}

void
hclient_t::run ()
{
  if (noisy) warn << "run: " << id << "\n";
  if (nrunning >= nconcur) {
    if (noisy) warn << "queuing: " << id << "\n";
    q.push_back (this);
    return;
  }
  nrunning++;
  if (noisy) warn << "running: " << id << " (nrunning: " << nrunning << ")\n";
  if (noisy) warn << "connecting to: " << host << "\n";
  cli_start = tsnow;
  tcpconnect (host, port, wrap (this, &hclient_t::connected));
}

static void
usage ()
{
  fatal << "usage: ptcli [-d] [-{s|p|o}] [-c <concur>] [-n <num>] <host>\n";
}

static void
main2 (str host, int port, str in, int n)
{
  for (int i = 0; i < n; i++) {
    hclient_t *h = New hclient_t (host, port, in, i);
    h->run ();
  }
}

int 
main (int argc, char *argv[])
{
  bool noisy = false;
  srandom(time(0));
  setprogname (argv[0]);
  int ch;
  int n = 1000;
  nconcur = 500; 
  bool delay = false;
  timespec startat;
  startat.tv_nsec = 0;

  while ((ch = getopt (argc, argv, "dc:n:spot:")) != -1) {
    switch (ch) {
    case 'd':
      noisy = true;
      break;
    case 'c':
      if (!convertint (optarg, &nconcur))
	usage ();
      if (noisy) warn << "Concurrency factor: " << nconcur << "\n";
      break;
    case 'n':
      if (!convertint (optarg, &n))
	usage ();
      if (noisy) warn << "Number of requests: " << n << "\n";
      break;
    case 's':
      mode = SEDA;
      break;
    case 'p':
      mode = PHP;
      break;
    case 'o':
      mode = OKWS;
      break;
    case 't': 
      {
	if (!convertint (optarg, &startat.tv_sec))
	  usage ();
	delay = true;
	if (noisy) warn << "Delaying start until time=" 
			<< startat.tv_sec << "\n";
	time_t mytm = time (NULL);
	int tmp =  startat.tv_sec - mytm;
	if (tmp < 0) {
	  warn << "time stamp alreached (it's " << mytm << " right now)!\n";
	  usage ();
	}
	if (noisy) {
	  warn << "Starting in T minus " << tmp << " seconds\n";
	}
	break;
      }
    default:
      usage ();
    }
  }
  argc -= optind;
  argv += optind;

  if (argc != 1)
    usage ();

  str dest = argv[0];

  str in;
  switch (mode) {
  case SEDA:
    in = "GET /test?x=";
    if (noisy) warn << "In SEDA mode\n";
    break;
  case OKWS:
    in = "GET /pt1?id=";
    if (noisy) warn << "In OKWS mode\n";
    break;
  case PHP:
    in = "GET /pt1.php?id=";
    if (noisy) warn << "In PHP mode\n";
    break;
  default:
    warnx << "no operation mode selected\n";
    usage ();
    break;
  }

  if (!hostport.match (dest)) 
    usage ();
  str host = hostport[1];
  str port_s = hostport[3];
  int port;
  if (port_s) {
    if (!convertint (port_s, &port)) usage ();
  } else {
    port = 80;
  }

  // unless we don this, shit won't be initialized, and i'll
  // starting ripping my hair out as to why all of the timestamps
  // are negative
  clock_gettime (CLOCK_REALTIME, &tsnow);

  nrunning = 0;
  sdflag = true;
  nreq = n;
  nreq_fixed = n;
  startt = tsnow;

  if (delay) {
    timecb (startat, wrap (main2, host, port, in, n));
  } else {
    main2 (host, port, in, n);
  }
  amain ();
}

