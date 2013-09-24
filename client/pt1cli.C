// $Id$

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

#define THOUSAND 1000
#define MILLION  ( THOUSAND * THOUSAND )

struct stats_t {
  stats_t () 
    : empties (0), timeouts (0), read_errors (0), rejections (0),
      exit_timeouts (0) {}

  int empties;
  int timeouts;
  int read_errors;
  int rejections;
  int exit_timeouts;
  void dump ()
  {
    warnx ("FAILURES: empties=%d; timeouts=%d; read_errors=%d; "
	   "rejections=%d; etimeouts=%d; total=%d\n",
	   empties, timeouts, read_errors, rejections, exit_timeouts,
	   empties + timeouts + read_errors + rejections + exit_timeouts);
  }

};
stats_t stats;

// A request is of the following form:
//
//   http://<host>:<port>/<U1><svc-id><U2><req-id><U3>
//

static rxx hostport ("([^:]+)(:(\\d+))?");
static rxx colon_rxx (":");
int nreq;
int nreq_fixed;
int nconcur;
int nrunning;
bool sdflag;
bool zippity;
bool noisy;
bool exited;
int nleft;
str host;
int port;
str inreq;
str inqry;
str suffix;
int req_modulus;
int hclient_id;
int latency_gcf;
int num_services;
int timeout;
bool use_latencies;
int nusers;
bool cycle_svc;
bool cycle_users;
int data_size;

vec<str> uri_parts;

//
// used for getting Service IDs or CGI parameter IDS, too, depending
// on what we need them for.
//
class id_cycler_t 
{
public:
  id_cycler_t () : _disabled (true) {}
  id_cycler_t (bool r, int m, int o) 
    : _disabled (false), _rand (r), _modulus (m), _i (0), _offset (o) {}
  
  // possible inputs:
  //
  //  400+c50  =>  400, 401, 402 ... 449, 400, ....
  //  400+r50  =>  443, 412, 403, 412, 414, 439 ... (i.e., random)
  //  400      =>  400, 400, 400, 400, ....
  //           =>  -1, -1, -1, ....
  //
  bool 
  init (const str &s)
  {
    static rxx plus_rxx ("\\+");
    _disabled = true;

    if (s.len () == 0) 
      return true;

    vec<str> parts;
    if (split (&parts, plus_rxx, s) < 1)
      return false;

    // the first part of the expression is the constant offset
    if (parts.size () >= 1) {
      _disabled = false;
      _modulus = 1;
      _rand = false;
      if (!convertint (parts[0], &_offset))
	return false;
    } 

    // the second part tells how to cycle IDs.  note that it is optional
    if (parts.size () == 2) {
      const char *cp = parts[1].cstr();
      switch (*cp) {
      case 'c':
	_rand = false;
	break;
      case 'r':
	_rand = true;
	break;
      default:
	return false;
      }
      cp ++;
      if (!convertint (cp, &_modulus))
	return false;

    }
    if (parts.size () > 2)
      return false;
    return true;
  }

  int nxt () 
  {
    int ret = -1;
    if (!_disabled) {
      if (_modulus == 1) 
	ret = 0;
      else if (_rand)
	ret = random ();
      else
	ret = _i ++;
      ret = (ret % _modulus) + _offset;
    }
    return ret;
  }

private:
  bool _disabled;
  bool _rand;
  int  _modulus;
  int _i;
  int _offset;
};

vec<id_cycler_t *> id_cyclers;

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
int tpt_sample_period_secs;
int tpt_sample_period_nsecs;
void tpt_do_sample (bool last);
int n_successes;
int last_n_successes;
int lose_patience_after; // end the run if we lost patience
int n_still_patient; // if more than n outstanding, we're still patient

timespec startt;
timespec lastexit;

typedef 
enum { NONE = 0, OKWS = 1, PHP = 2, SEDA = 3, FLASH = 4 } pt1cli_mode_t;

pt1cli_mode_t mode;

void global_do_exit (int rc);
int stop_timer (const timespec &t);
vec<int> latencies;
vec<exit_pair_t> exits;
normdist_t *dist;

static int
get_cli_write_delay ()
{
  return use_latencies ? dist->sample () : 0;
}

static str
get_uri_after_port ()
{
  // the ids are smushed between URI parts
  assert (id_cyclers.size () <= uri_parts.size ());
  u_int i;
  strbuf b;
  for (i = 0; i < id_cyclers.size (); i++) {
    b << uri_parts[i];
    int tmp = id_cyclers[i]->nxt ();
    if (tmp >= 0)
      b << tmp;
  }
  if (i < uri_parts.size ())
    b << uri_parts[i];
  return b;
}

class hclient_t {
public:
  hclient_t () 
  {
    init ();
  }

  void init ()
  {
    id = hclient_id++;
    bp = buf; 
    reqsz = -1;
    body = NULL;
    success = false;
    output_stuff  = false;
    selread_on  = true;
    tcb  = NULL ;
    wcb  = NULL;
    destroyed  = New refcounted<bool> (false);
  }

  ~hclient_t () 
  {
    close ();
    *destroyed = true;
  }

  void run ();
  void timed_out (ptr<bool> d);

private:
  void launch_others ();
  void connected (ptr<bool> d_local, int f);
  void canread ();
  void do_read ();
  void hclient_do_exit (int c);
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
  char buf[8192];
  int fd;
  int id;
  char *bp;
  int reqsz;
  char *body;
  suio uio;
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
      h = New hclient_t ();
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
  hclient_do_exit (c);
}

void
hclient_t::hclient_do_exit (int c)
{
  if (c == 0)
    n_successes ++;

  lastexit = sfs_get_tsnow();

  if (noisy) warn << "at cexit2: " << id << "\n";
  --nrunning;
  launch_others ();
  if (noisy) warn << "done: " << id << " (nreq: " << nreq << ")\n";
  if (success)
    latencies.push_back (stop_timer (cli_start));
  if (!--nreq && sdflag)
    global_do_exit (c);
  delete this;
  return;
}

static rxx sz_rxx ("[C|c]ontent-[l|L]ength: (\\d+)\\r\\n");

void
hclient_t::canread ()
{
  do_read ();
}

void
hclient_t::do_read ()
{
  int rc = uio.input (fd);
  if (rc == 0 && !output_stuff) {
    stats.empties ++;
    warn ("no content before EOF\n");
    cexit (-1);
    return;;
  } else if (rc == 0) {
    // legitimate EOF
    cexit (0);
    return;
  } else if (rc == -1) {
    if (errno != EAGAIN) {
      stats.read_errors ++ ;
      warn ("read error: %m\n");
      cexit (-1);
    }
    return;
  }

  assert (rc > 0);

  // for sane Web servers, this is easy...
  if (mode != SEDA) {
    while (uio.resid ()) {
      output_stuff = true;
      uio.output (1);
    }
    return;
  }

  //
  // for SEDA, we've had to parse the header and cut off the
  // connection, since it doesn't support closed connections
  // as we've asked.
  //
  rc = uio.copyout (bp);
  uio.rembytes (rc);
  bp += rc;
  
  //
  // reqsz < 0 if we haven't found a field yet in the header
  //
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
  
  //
  // if we've seen the required number of bytes (as given by
  // the header) then we're ready to close (or rock!)
  //
  if (body && (bp - body >= reqsz)) {
    rc_ignore (write (1, buf, bp - buf));
    if (noisy) warn ("ok, closing up!\n");
    cexit (0);
  }
}

void 
hclient_t::connected (ptr<bool> d_local, int f)
{
  if (*d_local) {
    if (f > 0)
      ::close (f);
    stats.timeouts ++;
    warn << "timed out before connect succeeded\n";
    return;
  }

  fd = f;

  if (fd < 0) {
    stats.rejections ++;
    warn << "connection rejected\n";
    cexit (-1);
    return;
  }
  int d = get_cli_write_delay ();
  if (d == 0)
    sched_write (destroyed);
  else {
    wcb = delaycb (d / THOUSAND, (d % THOUSAND) * MILLION, 
		   wrap (this, &hclient_t::sched_write, destroyed));
  }
}

void
hclient_t::sched_write (ptr<bool> d_local)
{
  wcb = NULL;
  if (*d_local) {
    warn << "destroyed before writing possible\n";
    return;
  }

  strbuf b;
  int v = zippity ? 1 : 0;

  b << "GET /" << get_uri_after_port () << " HTTP/1." << v << "\r\n";

  if (zippity) 
    b << "Accept: */*\r\n"
      << "Accept-Languge: en-us\r\n"
      << "Accept-Encoding: gzip, deflate\r\n"
      << "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; "
      << "Windows NT 5.1; .NET CLR 1.0.3705)\r\n"
      << "Host: " << host << "\r\n";

  b << "Connection: close\r\n\r\n";

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

// output all measurements in pretty cryptic formats.  we detail those
// formats inline.
void
global_do_exit (int c)
{
  int total_tm = stop_timer (startt);

  // output the command line parameters used for this experiment
  // (the number of concurrent clients, the number of total requests,
  // and the time it takes to run the whole experiment).  note that
  // the total time can be misleading, since there might be timeouts
  // (which default to 120 seconds if the server does not respond)
  warnx << nconcur << "," << nreq_fixed << "," << total_tm << "\n";

  // first print out the latencies.  there is one line here for each
  // request.  the value is the number of microseeconds it takes for the
  // request to complete.
  u_int sz = latencies.size ();
  for (u_int i = 0; i < sz; i++) 
    warnx << latencies[i] << "\n";

  // after we're done outputting the latencies, we output the throughputs
  // measured throught the experiment.  we sample throughput once per
  // second, and output the number of requests served in that second.
  // the output is one pair (on a line) per sample.  
  // the first part of the pair is the number of requests completed
  // in the sample interval.
  // the second part of the pair is the number of microsends in the 
  // sample (since we'll never have a 1 second sample exactly).
  tpt_do_sample (true);
  sz = tpt_samples.size ();
  for (u_int i = 0; i < sz; i++)
    warnx << tpt_samples[i].nreq << "," << tpt_samples[i].tm << "\n";

  // output error statistics
  stats.dump ();

  exit (c);
}

// returns a time in us
static int timeval(const timespec &t) {
  return t.tv_nsec / THOUSAND + t.tv_sec * MILLION;
}

static int timediff (const timespec &t1, const timespec &t2)
{
  return ((timeval (t1) - timeval (t2)) / MILLION);
}

// returns a time in microseconds
int
stop_timer (const timespec &tm)
{
  struct timespec tsnow = sfs_get_tsnow ();

  int ret = (tsnow.tv_nsec - tm.tv_nsec) / THOUSAND;
  int tmp = (tsnow.tv_sec - tm.tv_sec) * MILLION;
  ret += tmp;
  return ret;
}

void
hclient_t::run ()
{
  struct timespec tsnow = sfs_get_tsnow ();
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
  tcb = delaycb (timeout, 0, wrap (this, &hclient_t::timed_out, destroyed));
  tcpconnect (host, port, wrap (this, &hclient_t::connected, destroyed));
}

void
hclient_t::timed_out (ptr<bool> d_local) 
{
  stats.timeouts ++;
  if (*d_local) {
    warn << "unexpected timeout -- XXX\n";
    return;
  }

  warn << "client timed out: " << id << "\n";
  tcb = NULL;
  cexit (-1);
}

static void
usage ()
{
  warnx << "usage: " << progname << " [ -cdlmnrtvzMPST ] "
	<< "<host>:<port> [<custom> ... ]\n"
	<< "  Description of flags (default values in []'s)\n"
	<< "     -c<i>: set the number of concurrent connections to i [1]\n"
	<< "     -d   : turn on debug statements [false]\n"
	<< "     -l   : use random client latencies (normally distributed) "
	<< "[false]\n"
	<< "     -m<s>: Give one of { s : seda, p : php, o : okws, "
	<<                        "f : flash }\n"
	<< "            Will auto-construct URLs for tests [None]\n"
	<< "     -n<i>: set the total number of connections to make to "
	<< "i [1000]\n"
	<< "     -p   : don't use pub -- use static C text [false]\n"
	<< "     -r<i>: like -R 1+r<i>\n"
	<< "     -t<i>: start run at time i (UNIX timestamp)\n"
	<< "     -v<i>: like -V 1+r<i>\n"
	<< "     -M<i>: if using latencies, set mean = i [75msec]\n"
	<< "     -P<i>: Specifiy sampling period in msec [1000]\n"
	<< "     -S<i>: if using latencies, set stddev=i [25]\n"
	<< "     -R<s>: In auto-construct mode, params for cycling "
	<< "through IDs [1+r30000]\n"
	<< "   -T<i,j>: lose patience if fewer than i left and j "
	<< "secs of quiet elapsed []\n"
	<< "     -V<s>: In auto-construct mode, parameters for cycling "
	<< "through services [1]\n"
    ;
  exit (0);
}

static void
sched_tpt_measurement ()
{
  delaycb (tpt_sample_period_secs, tpt_sample_period_nsecs, 
	   wrap (tpt_do_sample, false));
}

void
tpt_do_sample (bool last)
{
  int diff = stop_timer (tpt_last_sample);
  int num = n_successes - last_n_successes;
  last_n_successes = n_successes;

  tpt_samples.push_back (tpt_pair_t (diff, num));
  tpt_last_sample = sfs_get_tsnow ();
  if (!last)
    sched_tpt_measurement ();
}

static void
main2 (int n)
{
  struct timespec tsnow = sfs_get_tsnow ();

  lastexit = tsnow;
  startt = tsnow;
  nleft = n - nconcur;
  
  tpt_last_sample = tsnow;
  sched_tpt_measurement ();

  for (int i = 0; i < nconcur; i++) {
    hclient_t *h = New hclient_t ();
    h->run ();
  }
}

static void schedule_lose_patience_timer ();

static void
lose_patience_cb ()
{
  struct timespec tsnow = sfs_get_tsnow ();
  if (noisy)
    warnx ("lpcb: td=%d; nleft=%d\n", 
	   timediff (tsnow, lastexit), nleft);
  if (lose_patience_after &&  
      timediff (tsnow, lastexit) > lose_patience_after &&
      n_still_patient &&
      nreq < n_still_patient) {
    stats.exit_timeouts += nreq;
    global_do_exit (0);
  } else
    schedule_lose_patience_timer ();
}

void
schedule_lose_patience_timer ()
{
  delaycb (1, 0, wrap (lose_patience_cb));
}


int 
main (int argc, char *argv[])
{
  timeout = 120;
  noisy = false;
  zippity = false;
  srandom(time(0));
  setprogname (argv[0]);
  int ch;
  int n = 1000;
  nconcur = 500; 
  bool delay = false;
  timespec startat;
  startat.tv_nsec = 0;
  startat.tv_sec = 0;
  exited = false;
  hclient_id = 1;
  use_latencies = false;
  num_services = 1;
  tpt_sample_period_secs = 1;
  tpt_sample_period_nsecs = 0;
  int lat_stddv = 25;
  int lat_mean = 75;
  lose_patience_after = 0;
  id_cycler_t *svc_cycler = NULL;
  id_cycler_t *req_cycler = NULL; 
  mode = NONE;
  bool no_pub = false;

  int tmp = 0;

  static rxx lose_patience_rxx ("(\\d+),(\\d+)");

  while ((ch = getopt (argc, argv, "c:dlm:n:pr:t:v:zM:P:S:R:T:V:")) != -1) {
    switch (ch) {

    case 'c':
      if (!convertint (optarg, &nconcur))
	usage ();
      if (noisy) warn << "Concurrency factor: " << nconcur << "\n";
      break;

    case 'd':
      noisy = true;
      break;

    case 'l':
      use_latencies = true;
      if (noisy) warn << "Using Latencies\n";
      break;

    case 'm':
      {
	switch (optarg[0]) {
	case 's':
	case 'S':
	  mode = SEDA;
	  if (noisy) warn << "In SEDA mode\n";
	  break;
	case 'o':
	case 'O':
	  mode = OKWS;
	  if (noisy) warn << "In OKWS mode\n";
	  break;
	case 'P':
	case 'p':
	  mode = PHP;
	  if (noisy) warn << "In PHP mode\n";
	  break;
	case 'f':
	case 'F':
	  mode = FLASH;
	  if (noisy) warn << "In FLASH mode\n";
	  break;
	default:
	  usage ();
	  break;
	}
	break;
      }
	
    case 'n':
      if (!convertint (optarg, &n))
	usage ();
      if (noisy) warn << "Number of requests: " << n << "\n";
      break;

    case 'p':
      no_pub = true;
      break;

    case 'r':
      if (!convertint (optarg, &tmp))
	usage ();
      req_cycler = New id_cycler_t (true, tmp, 1);
      if (noisy) 
	warn << "Ranging ids from 1 to " << tmp << " (randomly)\n";
      break;

    case 't': 
      {
	if (!convertint (optarg, &startat.tv_sec))
	  usage ();
	delay = true;
	if (noisy) warn << "Delaying start until time=" 
			<< startat.tv_sec << "\n";
	time_t mytm = time (NULL);
	tmp =  startat.tv_sec - mytm;
	if (tmp < 0) {
	  warn << "time stamp alreached (it's " << mytm << " right now)!\n";
	  usage ();
	}
	if (noisy) {
	  warn << "Starting in T minus " << tmp << " seconds\n";
	}
	break;
      }

    case 'v':
      if (!convertint (optarg, &tmp))
	usage ();
      svc_cycler = New id_cycler_t (true, tmp, 1);
      if (noisy) 
	warn << "Randing services from 1 to " << tmp << " (randomly)\n";
      break;

    case 'z':
      zippity = true;
      break;

    case 'M':
      if (!convertint (optarg, &lat_mean))
        usage ();
      if (noisy) warn << "Mean of latencies: " << lat_mean << "\n";
      break;

    case 'P':
      if (!convertint (optarg, &tmp))
	usage ();
      tpt_sample_period_secs = tmp / THOUSAND;
      tpt_sample_period_nsecs = (tmp % THOUSAND) * MILLION;
      if (noisy)
	warn ("Sample throughput period=%d.%03d secs\n", 
	      tpt_sample_period_secs,
	      tpt_sample_period_nsecs / MILLION);
      break;

    case 'R':
      req_cycler = New id_cycler_t ();
      if (!req_cycler->init (optarg))
	usage ();
      break;

    
    case 'S':
      if (!convertint (optarg, &lat_stddv))
        usage ();
      if (noisy) warn << "Standard dev. of latency: " << lat_stddv << "\n";
      break;

    case 'T':
      if (!lose_patience_rxx.match (optarg) ||
	  !convertint (lose_patience_rxx[1], &n_still_patient) ||
	  !convertint (lose_patience_rxx[2], &lose_patience_after))
	usage ();
      break;

    case 'V':
      svc_cycler = New id_cycler_t ();
      if (!svc_cycler->init (optarg))
	usage ();
      break;

    default:
      usage ();
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 0)
    usage ();

  str dest = argv[0];
  argc --;
  argv ++;

  // make the appropriate cyclers...
  if (argc > 0) {

    // in this case, the user supplied extra arguments after the hostname
    // and port; therefore, they're going to be making their own URL
    // by alternating static parts and cyclers.
    if (req_cycler) {
      warn << "Don't provide -r if you're going to make your own URI\n";
      usage ();
    }
    if (svc_cycler) {
      warn << "Don't provide -v if you're going to make your own URI\n";
      usage ();
    }

    for (int i = 0; i < argc; i++) {
      if (i % 2 == 0) {
	uri_parts.push_back (argv[i]);
      } else {
	id_cycler_t *tmp = New id_cycler_t ();
	if (!tmp->init (argv[i])) {
	  warn << "Cannot parse ID cycler: " << argv[i] << "\n";
	  usage ();
	}
	id_cyclers.push_back (tmp);
      }
    }

  } else if (mode != NONE) {
    // no manual URL building required; just specify some defaults
    // though if none were specified
    if (!req_cycler) 
      // roughly a million, but this way all reqs will have the same
      // number of digits
      req_cycler = New id_cycler_t (true, 900000, 100000);
    if (!svc_cycler)
      // don't cycle --- just always return 1
      svc_cycler = New id_cycler_t (false, 1, 1);

    id_cyclers.push_back (svc_cycler);
    id_cyclers.push_back (req_cycler);

    switch (mode) {
    case SEDA:
      uri_parts.push_back ("mt");
      uri_parts.push_back ("?id=");
      break;
    case OKWS: 
      {
	uri_parts.push_back ("mt"); 
	strbuf b ("?");
	if (no_pub) 
	  b << "nopub=1&";
	b << "id=";
	uri_parts.push_back (b);
	break;
      }
    case PHP:
      uri_parts.push_back ("mt");
      uri_parts.push_back (".php?id=");
      break;
    case FLASH:
      uri_parts.push_back ("cgi-bin/mt");
      uri_parts.push_back ("?");
      break;
    default:
      break;
    }
  }

  // normdist (mean, std-dev, "precision")
  if (use_latencies)
    dist = New normdist_t (200,25);

  if (!hostport.match (dest)) 
    usage ();
  host = hostport[1];
  str port_s = hostport[3];
  if (port_s) {
    if (!convertint (port_s, &port)) usage ();
  } else {
    port = 80;
  }

  struct timespec tsnow = sfs_get_tsnow ();

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


