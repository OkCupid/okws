#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "stdlib.h"
#include "time.h"

struct exit_pair_t {
  exit_pair_t (int q, int t) : qsz (q), tm (t) {}
  int qsz;
  int tm;
};

static rxx hostport ("([^:]+)(:(\\d+))?");
int nreq;
int nreq_fixed;
int nconcur;
int nrunning;
bool sdflag;
bool noisy;
bool exited;
int nleft;
str host;
int port;
str inreq;
str inqry;
int rand_modulus;
int hclient_id;
int num_latencies;
int latency_gcf;
int num_services;

timespec startt;
timespec lastexit;

typedef enum { OKWS = 1, PHP = 2, SEDA = 3, FLASH = 4 } pt1cli_mode_t;
pt1cli_mode_t mode;

void do_exit (int rc);
int stop_timer (const timespec &t);
vec<int> latencies;
vec<exit_pair_t> exits;

int
get_cli_read_delay (int id)
{
  return ((id % num_latencies) * latency_gcf);
}

int
get_svc_id ()
{
  return (random () % num_services);
}

class hclient_t {
public:
  hclient_t (str h, int p, str r, str q) 
    : host (h), port (p), req (r), id (hclient_id++), 
      bp (buf), reqsz (-1), body (NULL), 
      cli_read_delay (get_cli_read_delay (id)), qry (q) {}
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
  void launch_others ();
  void connected (int f);
  void canread ();
  void then_read_damnit ();
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
  const int cli_read_delay;
  str qry;
};

vec<hclient_t *> q;

void
hclient_t::launch_others ()
{
  hclient_t *h;
  while (nrunning < nconcur && nleft) {
    if (q.size ()) {
      if (noisy) warn << "launched queued req: " << id << "\n";
      h = q.pop_front ();
    } else {
      h = New hclient_t (host, port, inreq, inqry);
      if (noisy) warn << "alloc/launch new req: " << id << "\n";
      nleft --;
    }
    h->run ();
  }
}

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

  if (exited) {
    exits.push_back (exit_pair_t (nconcur - nrunning, stop_timer (lastexit)));
    lastexit = tsnow;
  } else {
    exited = true;
  }

  --nrunning;
  launch_others ();
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
  if (cli_read_delay == 0)
    then_read_damnit ();
  else
    delaycb (0, cli_read_delay, wrap (this, &hclient_t::then_read_damnit ));
}


void
hclient_t::then_read_damnit ()
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
  strbuf b;
  int id = random() % rand_modulus;
  b << "GET /" << req << get_svc_id () << qry << id << " HTTP/1.0\r\n"
    << " HTTP/1.0\r\nConnection: close\r\n\r\n";
  if (fd < 0) {
    if (mode != OKWS) {
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
  warnx << nconcur << "," << nreq_fixed << "," << total_tm << "\n";
  u_int sz = latencies.size ();
  for (u_int i = 0; i < sz; i++) 
    warnx << latencies[i] << "\n";
  sz = exits.size ();
  for (u_int i = 0; i < sz; i++)
    warnx << exits[i].qsz << "," << exits[i].tm << "\n";
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
main2 (int n)
{
  startt = tsnow;
  nleft = n - nconcur;
  for (int i = 0; i < nconcur; i++) {
    hclient_t *h = New hclient_t (host, port, inreq, inqry);
    h->run ();
  }
}

int 
main (int argc, char *argv[])
{
  noisy = false;
  srandom(time(0));
  setprogname (argv[0]);
  int ch;
  int n = 1000;
  nconcur = 500; 
  bool delay = false;
  timespec startat;
  startat.tv_nsec = 0;
  exited = false;
  rand_modulus = 30000;
  hclient_id = 1;
  latency_gcf = 50;
  num_latencies = 0;
  num_services = 1;

  while ((ch = getopt (argc, argv, "spofdc:n:t:r:v:l:L:")) != -1) {
    switch (ch) {
    case 'r':
      if (!convertint (optarg, &rand_modulus))
	usage ();
      if (noisy) warn << "Random modulus: " << rand_modulus << "\n";
      break;
    case 'v':
      if (!convertint (optarg, &num_services))
	usage ();
      if (noisy) warn << "Num Services: " << num_services << "\n";
      break;
    case 'l':
      if (!convertint (optarg, &num_latencies))
	usage ();
      if (noisy) warn << "Num Latencies: " << num_latencies << "\n";
      break;
    case 'L':
      if (!convertint (optarg, &latency_gcf))
	usage ();
      if (noisy) warn << "Latency GCF: " << latency_gcf << "\n";
      break;
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
    case 'f':
      mode = FLASH;
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

  switch (mode) {
  case SEDA:
    inreq = "test";
    inqry = "?id=";
    if (noisy) warn << "In SEDA mode\n";
    break;
  case OKWS:
    inreq = "pt";
    inrqry = "?id=";
    if (noisy) warn << "In OKWS mode\n";
    break;
  case PHP:
    inreq = "pt";
    inqry = ".php?id=";
    if (noisy) warn << "In PHP mode\n";
    break;
  case FLASH:
    inreq = "/cgi-bin/d_reg";
    inqry = "?";
    if (noisy) warn << "In FLASH mode\n";
    break;
  default:
    warnx << "no operation mode selected\n";
    usage ();
    break;
  }

  if (!hostport.match (dest)) 
    usage ();
  host = hostport[1];
  str port_s = hostport[3];
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

  if (delay) {
    timecb (startat, wrap (main2, n));
  } else {
    main2 (n);
  }
  amain ();
}

