#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "stdlib.h"
#include "time.h"
#include "normdist.h"

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
int timeout;

//
// used for sample throughput
//
struct tpt_pair_t {
  tpt_pair_t (int d, int t) : nreq (d), tm (t) {}
  int nreq;
  int tm;
};

timespec tpt_last_sample;
int tpt_last_nreq;
vec<tpt_pair_t> tpt_samples;
int tpt_sample_freq;
void tpt_do_sample (bool last);

timespec startt;
timespec lastexit;

typedef enum { OKWS = 1, PHP = 2, SEDA = 3, FLASH = 4 } pt1cli_mode_t;
pt1cli_mode_t mode;

void do_exit (int rc);
int stop_timer (const timespec &t);
vec<int> latencies;
vec<exit_pair_t> exits;
normdist_t *dist;

int
get_cli_write_delay (int id)
{
  if (num_latencies == 0)
    return 0;
  return dist->sample ();
}

int
get_svc_id (int id)
{
  return ((id  % num_services) + 1);
}

class hclient_t {
public:
  hclient_t (str h, int p, str r, str q) 
    : host (h), port (p), req (r), id (hclient_id++), 
      bp (buf), reqsz (-1), body (NULL), 
      cli_write_delay (get_cli_write_delay (id)), qry (q), success (false),
      output_stuff (false), selread_on (true), tcb (NULL), wcb (NULL),
      destroyed (New refcounted<bool> (false)) {}
  ~hclient_t () 
  {
    close ();
    *destroyed = true;
  }

  void run ();
  void timed_out ();

private:
  void launch_others ();
  void connected (int f);
  void canread ();
  void then_read_damnit ();
  void then_exit_damnit (int c);
  void writewait ();
  void cexit (int rc);
  void sched_read ();
  void sched_write (ptr<bool> d);

  void turn_off ()
  {
    if (selread_on && fd >= 0) {
      fdcb (fd, selread, NULL);
      selread_on = false;
      success = true;
    } else {
      success = false;
    }
  }

  void close ()
  {
    if (fd >= 0) {
      turn_off ();
      // close (fd);
      tcp_abort (fd);
      fd = -1;
    }
  }
    
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
  const int cli_write_delay;
  str qry;
  bool success;
  bool output_stuff;
  bool selread_on;
  timecb_t *tcb, *wcb;
  ptr<bool> destroyed;
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
  if (tcb) {
    timecb_remove (tcb);
    tcb = NULL;
  }
  if (wcb) {
    timecb_remove (wcb);
    wcb = NULL;
  }

  if (noisy) warn << "at cexit: " << id << "\n";
  turn_off ();
  then_exit_damnit (c);
}

void
hclient_t::then_exit_damnit (int c)
{
  if (noisy) warn << "at cexit2: " << id << "\n";
  --nrunning;
  launch_others ();
  if (noisy) warn << "done: " << id << " (nreq: " << nreq << ")\n";
  if (success)
    latencies.push_back (stop_timer (cli_start));
  if (!--nreq && sdflag)
    do_exit (c);
  delete this;
  return;
}

static rxx sz_rxx ("[C|c]ontent-[l|L]ength: (\\d+)\\r\\n");

void
hclient_t::canread ()
{
  then_read_damnit ();
}

void
hclient_t::then_read_damnit ()
{
  int rc = uio.input (fd);
  if (rc == 0) {
    if (!output_stuff)
      warn ("no content before EOF\n");
    cexit (0);
    return;
  }
  if (mode != SEDA) {
    if (rc == -1 && errno != EAGAIN) {
      warn ("read error: %m\n");
      cexit (-1);
    }
    while (uio.resid ()) {
      output_stuff = true;
      uio.output (1);
    }
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



  if (fd < 0) {
    warn << "connection rejected\n";
    cexit (-1);
    return;
  }

  ptr<bool> destroyed2 = destroyed;
  if (cli_write_delay == 0)
    sched_write (destroyed2);
  else {
    wcb = delaycb (cli_write_delay / 1000, (cli_write_delay % 1000) * 1000000, 
		   wrap (this, &hclient_t::sched_write, destroyed2));
  }
}

void
hclient_t::sched_write (ptr<bool> destroyed2)
{
  wcb = NULL;
  if (*destroyed2) {
    warn << "destroyed before writing possible\n";
    return;
  }

  strbuf b;
  int id = random() % rand_modulus;
  b << "GET /" << req << get_svc_id (id) << qry << id << " HTTP/1.0\r\n"
    << "Connection: close\r\n\r\n";
  // this might qualify as "deafening"
  if (noisy) warn << b << "\n";

  suio *uio = b.tosuio ();
  while (uio->resid ()) {
    writewait ();
    if (uio->output (fd) < 0)
      fatal << "write error\n";
  }
  sched_read ();
}

void
hclient_t::sched_read ()
{
  selread_on = true;
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
  tpt_do_sample (true);
  sz = tpt_samples.size ();
  for (u_int i = 0; i < sz; i++)
    warnx << tpt_samples[i].nreq << "," << tpt_samples[i].tm << "\n";
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
  tcb = delaycb (timeout, 0, wrap (this, (&hclient_t::timed_out)));
  tcpconnect (host, port, wrap (this, &hclient_t::connected));
}

void
hclient_t::timed_out ()
{
  warn << "client timed out: " << id << "\n";
  tcb = NULL;
  cexit (-1);
}

static void
usage ()
{
  fatal << "usage: ptcli [-d] [-{s|p|o}] [-c <concur>] [-n <num>] <host>\n";
}

void
tpt_do_sample (bool last)
{
  int diff = stop_timer (tpt_last_sample);
  int num = tpt_last_nreq - nreq;
  tpt_samples.push_back (tpt_pair_t (diff, num));
  tpt_last_sample = tsnow;
  tpt_last_nreq = nreq;
  if (!last)
    delaycb (tpt_sample_freq, 0, wrap (tpt_do_sample, false));
}

static void
main2 (int n)
{
  startt = tsnow;
  nleft = n - nconcur;
  
  tpt_last_sample = tsnow;
  delaycb (tpt_sample_freq, 0, wrap (tpt_do_sample, false));
  for (int i = 0; i < nconcur; i++) {
    hclient_t *h = New hclient_t (host, port, inreq, inqry);
    h->run ();
  }
}


int 
main (int argc, char *argv[])
{
  timeout = 180;
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
  tpt_sample_freq = 1;

  // normdist (mean, std-dev, "precision")
  dist = New normdist_t (100,35);
  dist->dump ();
  exit (0);

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
    inqry = "?id=";
    if (noisy) warn << "In OKWS mode\n";
    break;
  case PHP:
    inreq = "pt";
    inqry = ".php?id=";
    if (noisy) warn << "In PHP mode\n";
    break;
  case FLASH:
    inreq = "cgi-bin/d_reg";
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
  tpt_last_nreq = nreq;

  if (delay) {
    timecb (startat, wrap (main2, n));
  } else {
    main2 (n);
  }
  amain ();
}

