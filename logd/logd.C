
/* $Id$ */ 

#include "okprot.h"
#include "ok.h"
#include "okerr.h"
#include "rxx.h"
#include "svq.h"
#include "logd.h"

logd_t *glogd;

static void
usage ()
{
  warnx << "usage: oklogd <cgi-encoded-parameters>\n";
  exit (1);
}

void
logd_t::newclnt (bool p, ptr<axprt_stream> x)
{
  if (!x && p) 
    fatal ("Unexpected EOF from parent process.\n");
  lst.insert_tail (New logd_client_t (x, this, p));
}

logd_client_t::logd_client_t (ptr<axprt_stream> x, logd_t *d, bool p)
  : srv (asrv::alloc (x, oklog_program_1, 
		      wrap (this, &logd_client_t::dispatch))),
    logd (d), primary (p) {}

logd_client_t::~logd_client_t () { logd->remove (this); }

void
logd_client_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    if (primary) {
      warn << "EOF received from parent/master process\n";
      logd->shutdown ();
    } else {
      delete this;
      return;
    }
  }
  logd->dispatch (sbp);
}

void
logd_t::shutdown ()
{
  close_fdfd ();
  delete this;
  exit (0);
}

void
logd_t::dispatch (svccb *sbp)
{
  u_int p = sbp->proc ();
  switch (p) {
  case OKLOG_NULL:
    sbp->reply (NULL);
    break;
  case OKLOG_LOG:
    log (sbp);
    break;
  case OKLOG_GET_LOGSET:
    sbp->replyref (logset);
    break;
  case OKLOG_TURN:
    turn (sbp);
    break;
  case OKLOG_KILL:
    warn << "Caught kill RPC; shutting down\n";
    shutdown ();
    break;
  case OKLOG_CLONE:
    clonefd (sbp);
    break;
  case OKLOG_FAST:
    fastlog (sbp);
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
  return;
}


bool
logd_t::slave_setup ()
{
  if (pub_slave (wrap (this, &logd_t::newclnt, true)) != PSLAVE_SLAVE) {
    warn << "cannot establish slave socket\n";
    return false;
  }
  return true;
}

bool
logd_t::logfile_setup ()
{
  str prfx;
  if (!injail)
    prfx = parms.logdir;
  return (access.open_verbose (parms.accesslog, "access", prfx) &&
	  error.open_verbose (parms.errorlog, "error", prfx));
}

void
logd_t::close_logfiles ()
{
  error.close ();
  access.close ();
}

void
logfile_t::close ()
{
  if (fd >= 0) {
    flush ();
    ::close (fd);
    fd = -1;
  }
}

bool
logfile_t::flush (const str &x)
{
  if (!x || !x.len ())
    return false;
  suio uio;
  uio.print (x.cstr (), x.len ());
  int rc;
  while ((rc = uio.output (fd)) == -1 && errno == EAGAIN) ;
  return (rc >= 0);
}

bool
logfile_t::open_verbose (const str &n, const str &typ, const str &p)
{
  if (n) fn = n;
  if (!fn) {
    warn << "no " << typ << " log specified\n";
    return false;
  }

  str fullpath;
  if (p) {
    strbuf b (p);
    if (p[p.len () - 1] != '/' && fn[0] != '/')
      b << "/";
    b << fn;
    fullpath = b;
  } else {
    fullpath = fn;
  }
    
  if (!open (fullpath)) {
    warn << "cannot open log file: " << fullpath << "\n";
    return false;
  }
  return true;
}

void
logd_t::parse_fmt ()
{
  const char *fmt = parms.accesslog_fmt 
    ? parms.accesslog_fmt.cstr () : ok_access_log_fmt.cstr ();
  const char *fp;
  for (fp = fmt; *fp; fp++) {
    logd_fmt_el_t *el = NULL;
    switch (*fp) {
    case 't':
      el = New logd_fmt_time_t (this);
      break;
    case 'r':
      el = New logd_fmt_referer_t ();
      break;
    case 'i':
      el = New logd_fmt_remote_ip_t ();
      break;
    case 'u':
      el = New logd_fmt_ua_t ();
      break;
    case '1':
      el = New logd_fmt_req_t ();
      break;
    case 's':
      el = New logd_fmt_status_t ();
      break;
    case 'b':
      el = New logd_fmt_bytes_t ();
      break;
    case 'v':
      el = New logd_fmt_svc_t ();
      break;
    case 'U':
      el = New logd_fmt_uid_t ();
      break;
    default:
      el = New logd_fmt_const_t (*fp);
      break;
    }
    logset |= el->get_switch ();
    fmt_els.push_back (el);
  }
}

bool
logd_t::fdfd_setup ()
{
  if (fdfd < 0) {
    warn << "no socket setup for file descriptor passing!\n";
    return false;
  }
  return true;
}

void
logd_t::clonefd (svccb *b)
{
  bool ret = true;
  int fds[2];
  int rc = socketpair (AF_UNIX, SOCK_STREAM, 0, fds);
  if (rc < 0) {
    warn ("socketpair: %m\n");
    ret = false;
  } else {
    newclnt (false, axprt_unix::alloc (fds[0]));
    fdsendq.push_back (fdtosend (fds[1], true));
    fdcb (fdfd, selwrite, wrap (this, &logd_t::writefd));
  }
  b->replyref (ret);
}

void
logd_t::writefd ()
{
  ssize_t n = 0;
  while (fdsendq.size ()) {
    n = writevfd (fdfd, NULL, 0, fdsendq.front ().fd);
    if (n < 0)
      break;
    fdsendq.pop_front ();
  }
 
  if (n >= 0)
    fdcb (fdfd, selwrite, NULL);
}

void
logd_t::close_fdfd ()
{
  if (fdfd >= 0) 
    close (fdfd);
}

void
logd_t::launch ()
{
  if (!setup ()) 
    fatal << "launch/setup of failed\n";
}

bool
logd_t::setup ()
{
  parse_fmt ();
  if (!(perms_setup () && logfile_setup () && slave_setup () 
	&& fdfd_setup ())) 
    return false;
  timer_setup ();
  running = true;
  return true;
}

bool
logd_t::perms_setup ()
{
  if (!uid) {
    setgroups (0, NULL);
    if (!usr) {
      warn << "no valid run-as-user set while running as root\n";
      return false;
    }
    if (!grp) {
      warn << "no valid run-as-group set while running as root\n";
      return false;
    }
    if (parms.logdir) {
      if (chroot (parms.logdir) < 0) {
	warn << "chroot to directory failed: " << parms.logdir << "\n";
	return false;
      }
      injail = true;
    }
    if (setgid (grp.getid ()) != 0) {
      warn << "could not setgid to group: t" << grp.getname () << "\n";
      return false;
    }
    if (setuid (usr.getid ()) != 0) {
      warn << "could not setuid to user: " << usr.getname () << "\n";
      return false;
    }
  }
  return true;
}

void
logd_t::flush ()
{
  if (running) {
    error.flush ();
    access.flush ();
  }
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  int ch;
  vec<str> hosts;
  vec<str> files;
  int fdfd;
  while ((ch = getopt (argc, argv, "s:?")) != -1)
    switch (ch) {
    case 's':
      if (!convertint (optarg, &fdfd))
	usage ();
      break;
    case '?':
    default:
      usage ();
      break;
    }

  if (optind == argc) 
    usage ();

  setsid ();
  warn ("version %s, pid %d\n", VERSION, int (getpid ()));
  glogd = New logd_t (argv[optind], fdfd);
  glogd->launch ();
  amain ();
}


bool
logfile_t::open (const str &f)
{
  fd = ::open (f.cstr (), O_APPEND|O_CREAT|O_WRONLY, 0644);
  return (fd >= 0);
}

bool
logd_t::access_log (const oklog_ok_t &x)
{
  u_int lim = fmt_els.size ();
  logbuf_t *lb = access.getbuf ();
  for (u_int i = 0; i < lim; i++) {
    if (i != 0) lb->spc ();
    fmt_els[i]->log (lb, x);
  }
  lb->newline ();
  return lb->ok ();
}

bool
logd_t::error_log (const oklog_arg_t &x)
{
  logbuf_t *lb = error.getbuf ();
  (*lb) << tmr << ' ' << x.typ << ' ';
  switch (x.typ) {
  case OKLOG_ERR_NOTICE:
  case OKLOG_ERR_CRITICAL:
    (*lb) << x.notice->notice << '\n';
    break;
  default:
    {
      lb->copy ("[client ", 8).remote_ip (x.err->log.ip)
	.copy ("] ", 2).status (x.err->log.status);
      if (x.err->aux && x.err->aux.len ())
	lb->copy (": ", 2).copy (x.err->aux);
    }
  }
  lb->newline ();
  return lb->ok ();
}

void
logd_t::turn (svccb *sbp)
{
  bool ret = true;
  close_logfiles ();
  if (!logfile_setup ()) 
    ret = false;
  sbp->replyref (ret);
  if (!ret)
    fatal << "flush of logfiles failed\n";
}

void
logd_t::fastlog (svccb *sbp)
{
  bool ret = true;
  oklog_fast_arg_t *fa = sbp->template getarg<oklog_fast_arg_t> ();
  access.flush (fa->access);
  error.flush (fa->error);
  sbp->replyref (&ret);
}

void
logd_t::log (svccb *sbp)
{
  bool ret = true;
  oklog_arg_t *la = sbp->template getarg<oklog_arg_t> ();
  switch (la->typ) {
  case OKLOG_OK:
    ret = access_log (*la->ok);
    break;
  case OKLOG_ERR_NOTICE:
  case OKLOG_ERR_CRITICAL:
    ret = error_log (*la);
    break;
  default:
    if (!error_log (*la)) ret = false;
    if (!(la->err->log.req && la->err->log.req.len ())) 
      la->err->log.req = la->err->aux;
    if (!access_log (la->err->log)) ret = false;
    break;
  }
  sbp->replyref (ret);
}
