/* $Id$ */ 

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

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
  : srv (asrv_delayed_eof::alloc (x, oklog_program_1, 
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
  clone_server_t::close ();
  clean_pidfile ();
  delete this;
  exit (0);
}

void
logd_t::clean_pidfile ()
{
  if (_pidfile) {
    const char *f = _pidfile;
    int rc = unlink (f);
    if (rc != 0) {
      warn ("failed to clean pidfile ('%s'): %m\n", f);
    }
  }
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
logd_t::logfile_setup (logfile_t **f, const str &log, const str &typ)
{
  assert (!*f);
  *f = New logfile_t ();
  if ((*f)->open_verbose (fixup_file (log), typ)) {
    return true;
  } else {
    delete *f;
    *f = NULL;
    return false;
  }
}

bool
logd_t::logfile_setup ()
{
  return (logfile_setup (&access, parms.accesslog, "access") &&
	  logfile_setup (&error, parms.errorlog, "error") &&
	  logfile_setup (&ssl, parms.ssllog, "ssl"));
}


void
logd_t::close_logfiles ()
{
  delete access;
  delete error;
  delete ssl;
  access = NULL;
  error = NULL;
  ssl = NULL;
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
logfile_t::open_verbose (const str &n, const str &typ)
{
  if (!n) {
    warn << "no " << typ << " log specified\n";
    return false;
  }
    
  if (!open (n)) {
    warn << "cannot open log file: " << n << "\n";
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

void
logd_t::launch ()
{
  if (!setup ()) 
    fatal << "launch/setup failed\n";
}

bool
logd_t::setup ()
{
  parse_fmt ();
  if (!(perms_setup () && 
	logfile_setup () && 
	pidfile_setup () && 
	slave_setup () 
	&& clone_server_t::setup ()))
    return false;
  timer_setup ();
  
  running = true;
  return true;
}

bool
logd_t::pidfile_setup ()
{
  _pidfile = fixup_file (parms.pidfile);

  strbuf b;
  bool ret = true;
  const char *f = _pidfile;
  strbuf b2;
  b << getpid ();
  if (!str2file (f, b, 0644)) {
    warn ("Could not allocate pidfile ('%s'): %m\n", f);
    ret = false;
  }
  return ret;
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
      } else {
	rc_ignore (chdir ("/"));
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
    assert (error);
    assert (access);
    assert (ssl);
    error->flush ();
    access->flush ();
    ssl->flush ();
  }
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  int ch;
  vec<str> hosts;
  vec<str> files;
  int fdfd (0);
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
 
  if (fdfd < 0) {
    warn << "No -f parameter passed; one is required\n";
    usage ();
  }

  setsid ();
  warn ("OKWS version %s, pid %d\n", VERSION, int (getpid ()));
  glogd = New logd_t (argv[optind], fdfd);
  sigcb (SIGHUP, wrap (glogd, &logd_t::handle_sighup));
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
logd_t::ssl_log (const oklog_ssl_msg_t &x)
{
  logbuf_t *lb = ssl->getbuf ();
  lb->copy ("[client ", 8).remote_ip (x.ip).copy ("] ", 2).copy (x.cipher);
  if (x.msg.len ()) lb->copy (" ", 1).copy (x.msg);
  return true;
}

bool
logd_t::access_log (const oklog_ok_t &x)
{
  u_int lim = fmt_els.size ();
  assert (access);
  logbuf_t *lb = access->getbuf ();
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
  assert (error);
  logbuf_t *lb = error->getbuf ();
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

bool
logd_t::turn ()
{
  bool ret = false;

  logfile_t *access2 = NULL;
  logfile_t *error2 = NULL;
  logfile_t *ssl2 = NULL;

  if (logfile_setup (&access2, parms.accesslog, "access") &&
      logfile_setup (&error2, parms.errorlog, "error") &&
      logfile_setup (&ssl2, parms.errorlog, "ssl")) {

    ret = true;
    close_logfiles ();
    access = access2;
    error = error2;
    ssl = ssl2;

  } else {
   
    if (access2) delete access2;
    if (error2) delete error2;
    if (ssl2) return ssl2;
  } 

  // else access2 autodeleted

  if (!ret) 
    warn << "flush of logfiles failed; no changes made\n";
  return ret;
}

void
logd_t::turn (svccb *sbp)
{
  bool ret = turn ();
  sbp->replyref (ret);
}

void
logd_t::handle_sighup ()
{
  warn << "Received HUP signal (" << SIGHUP  << "); turning logs\n";
  bool ret = turn ();
  if (!ret) 
    warn << "XX log turn failed!\n";
}

void
logd_t::fastlog (svccb *sbp)
{
  bool ret = true;
  RPC::oklog_program_1::oklog_fast_srv_t<svccb> srv (sbp);
  const oklog_fast_arg_t *fa = srv.getarg ();

  assert (access);
  assert (error);
  assert (ssl);
  access->flush (fa->access);
  error->flush (fa->error);
  ssl->flush (fa->ssl);

  srv.reply (ret);
}

void
logd_t::log (svccb *sbp)
{
  bool ret = true;
  RPC::oklog_program_1::oklog_log_srv_t<svccb> srv (sbp);
  oklog_arg_t *la = srv.getarg ();
  switch (la->typ) {
  case OKLOG_OK:
    ret = access_log (*la->ok);
    break;
  case OKLOG_ERR_NOTICE:
  case OKLOG_ERR_CRITICAL:
    ret = error_log (*la);
    break;
  case OKLOG_SSL:
    ret = ssl_log (*la->ssl);
    break;
  default:
    if (!error_log (*la)) ret = false;
    if (!(la->err->log.req && la->err->log.req.len ())) 
      la->err->log.req = la->err->aux;
    if (!access_log (la->err->log)) ret = false;
    break;
  }
  srv.reply (ret);
}

//-----------------------------------------------------------------------

str
logd_t::fixup_file (const str &s) const
{
  if (!s) return NULL;

  str p = injail ? "/" : parms.logdir;

  strbuf b ;
  b << p;

  if (p[p.len () - 1] != '/' && s[0] != '/')
    b << "/";
  b << s;

  str ret = b;

  return ret;
}

//-----------------------------------------------------------------------
