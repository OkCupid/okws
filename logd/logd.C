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
#include "rxx.h"
#include "svq.h"

//-----------------------------------------------------------------------

class logfile_t {
public:
  logfile_t (const str &n) : fn (n), fd (-1) {}
  logfile_t () : fd (-1) {}
  void setfile (const str &n) { fn = n; }
  ~logfile_t () { close (); }
  bool open (const str &n);
  bool open_verbose (const str &n, const str &typ);
  bool flush (const str &s, bool nl = true);
  void flush () {}
  void close ();
private:
  str fn;
  int fd;
};

//-----------------------------------------------------------------------

class logd_t;
class logd_client_t {
public:
  logd_client_t (ptr<axprt_stream> x, logd_t *d, bool p);
  ~logd_client_t ();
  void dispatch (svccb *sbp);
  tailq_entry<logd_client_t> lnk;
private:
  ptr<asrv> srv;
  logd_t *logd;
  bool primary;
};

//-----------------------------------------------------------------------

class logd_t : public clone_server_t {
public:
  logd_t (ptr<axprt_unix> x, const str &in)
    : clone_server_t (x),
      parms (in), logset (0), error (NULL), access (NULL), ssl (NULL),
      dcb (NULL), 
      uid (getuid ()), usr (parms.user), grp (parms.group), running (false),
      injail (false) {}

  void launch ();
  void remove (logd_client_t *c) { lst.remove (c); }
  void newclnt (bool p, ptr<axprt_stream> x);
  void dispatch (svccb *sbp);
  void shutdown ();
  void handle_sighup ();
protected:

  // need to implement this to be a clone server
  void register_newclient (ptr<axprt_stream> x) { newclnt (false, x); }

private:
  void turn (svccb *sbp);
  bool turn ();
  void log (svccb *sbp);
  void log (const oklog_entry_t &e);
  void flush ();
  bool setup ();
  bool slave_setup ();
  bool pidfile_setup ();
  bool logfile_setup () ;
  bool logfile_setup (logfile_t **f, const str &l, const str &t);
  bool perms_setup ();
  void close_logfiles ();
  void clean_pidfile ();
  str fixup_file (const str &in) const;

  tailq<logd_client_t, &logd_client_t::lnk> lst;
  logd_parms_t parms;
  u_int logset;
  logfile_t *error, *access, *ssl;
  timecb_t *dcb;
  int uid;
  ok_usr_t usr;
  ok_grp_t grp;
  bool running;

  bool injail;
  str _pidfile;
};

//-----------------------------------------------------------------------

logd_t *glogd;

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: oklogd <cgi-encoded-parameters>\n";
  exit (1);
}

//-----------------------------------------------------------------------

void
logd_t::newclnt (bool p, ptr<axprt_stream> x)
{
  if (!x && p) 
    fatal ("Unexpected EOF from parent process.\n");
  lst.insert_tail (New logd_client_t (x, this, p));
}

//-----------------------------------------------------------------------

logd_client_t::logd_client_t (ptr<axprt_stream> x, logd_t *d, bool p)
  : srv (asrv::alloc (x, oklog_program_1, 
		      wrap (this, &logd_client_t::dispatch))),
    logd (d), primary (p) {}

//-----------------------------------------------------------------------

logd_client_t::~logd_client_t () { logd->remove (this); }

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

void
logd_t::shutdown ()
{
  clean_pidfile ();
  delete this;
  exit (0);
}

//-----------------------------------------------------------------------

void
logd_t::clean_pidfile ()
{
  if (_pidfile) {
    const char *f = _pidfile.cstr();
    int rc = unlink (f);
    if (rc != 0) {
      warn ("failed to clean pidfile ('%s'): %m\n", f);
    }
  }
}

//-----------------------------------------------------------------------

void
logd_t::dispatch (svccb *sbp)
{
  u_int p = sbp->proc ();
  switch (p) {
  case OKLOG_NULL:
    sbp->reply (NULL);
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
    clone_server_t::clonefd (sbp);
    break;
  case OKLOG_LOG:
    log (sbp);
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
  return;
}

//-----------------------------------------------------------------------


bool
logd_t::slave_setup ()
{
  if (pub_slave (wrap (this, &logd_t::newclnt, true)) != PSLAVE_SLAVE) {
    warn << "cannot establish slave socket\n";
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

bool
logd_t::logfile_setup ()
{
  return (logfile_setup (&access, parms.accesslog, "access") &&
	  logfile_setup (&error, parms.errorlog, "error") &&
	  logfile_setup (&ssl, parms.ssllog, "ssl"));
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

void
logfile_t::close ()
{
  if (fd >= 0) {
    ::close (fd);
    fd = -1;
  }
}

//-----------------------------------------------------------------------

bool
logfile_t::flush (const str &x, bool nl)
{
  if (!x || !x.len ())
    return false;
  suio uio;
  uio.print (x.cstr (), x.len ());
  if (nl) { uio.print ("\n", 1); }
  int rc;
  while ((rc = uio.output (fd)) == -1 && errno == EAGAIN) ;
  return (rc >= 0);
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

void
logd_t::launch ()
{
  if (!setup ()) 
    fatal << "launch/setup failed\n";
}

//-----------------------------------------------------------------------

bool
logd_t::setup ()
{
  if (!(perms_setup () && 
	logfile_setup () && 
	pidfile_setup () && 
	slave_setup ()))
    return false;
  
  running = true;
  return true;
}

//-----------------------------------------------------------------------

bool
logd_t::pidfile_setup ()
{
  _pidfile = fixup_file (parms.pidfile);

  strbuf b;
  bool ret = true;
  const char *f = _pidfile.cstr();
  strbuf b2;
  b << getpid ();
  if (!str2file (f, b, 0644)) {
    warn ("Could not allocate pidfile ('%s'): %m\n", f);
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

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
      if (chroot (parms.logdir.cstr()) < 0) {
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

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  int ch;
  vec<str> hosts;
  vec<str> files;
  int fdfd (0);
  while ((ch = getopt (argc, argv, "?")) != -1)
    switch (ch) {
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
  ptr<axprt_unix> x = axprt_unix::alloc (0, ok_axprt_ps);
  glogd = New logd_t (x, argv[optind]);
  sigcb (SIGHUP, wrap (glogd, &logd_t::handle_sighup));
  glogd->launch ();
  amain ();
}

//-----------------------------------------------------------------------


bool
logfile_t::open (const str &f)
{
  fd = ::open (f.cstr (), O_APPEND|O_CREAT|O_WRONLY, 0644);
  return (fd >= 0);
}

//-----------------------------------------------------------------------

bool
logd_t::turn ()
{
  bool ret = false;

  logfile_t *access2 = NULL;
  logfile_t *error2 = NULL;
  logfile_t *ssl2 = NULL;

  if (logfile_setup (&access2, parms.accesslog, "access") &&
      logfile_setup (&error2, parms.errorlog, "error") &&
      logfile_setup (&ssl2, parms.ssllog, "ssl")) {

    ret = true;
    close_logfiles ();
    access = access2;
    error = error2;
    ssl = ssl2;

  } else {
   
    if (access2) delete access2;
    if (error2) delete error2;
    if (ssl2) delete ssl2;
  } 

  // else access2 autodeleted

  if (!ret) 
    warn << "flush of logfiles failed; no changes made\n";
  return ret;
}

//-----------------------------------------------------------------------

void
logd_t::turn (svccb *sbp)
{
  bool ret = turn ();
  sbp->replyref (ret);
}

//-----------------------------------------------------------------------

void
logd_t::handle_sighup ()
{
  warn << "Received HUP signal (" << SIGHUP  << "); turning logs\n";
  bool ret = turn ();
  if (!ret) 
    warn << "XX log turn failed!\n";
}

//-----------------------------------------------------------------------

void
logd_t::log (const oklog_entry_t &le)
{
  logfile_t *log = NULL;
  switch (le.file) {
  case OKLOG_ACCESS: log = access; break;
  case OKLOG_ERROR: log = error; break;
  case OKLOG_SSL: log = ssl; break;
  case OKLOG_NONE: break;
  default: break;
  }
  if (log) {
    log->flush (opaque2str (le.data));
  }
}

//-----------------------------------------------------------------------

void
logd_t::log (svccb *sbp)
{
  bool ret = true;
  RPC::oklog_program_1::oklog_log_srv_t<svccb> srv (sbp);
  const oklog_arg_t *a = srv.getarg ();

  for (size_t i = 0; i < a->entries.size (); i++) {
    log (a->entries[i]);
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
