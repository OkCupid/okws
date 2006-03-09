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

#include "amt.h"
#include "txa_prot.h"

#define LONG_REPLY_TIME   2

mtd_thread_t::mtd_thread_t (mtd_thread_arg_t *a)
  : tid (a->tid), readied (false), fdin (a->fdin), fdout (a->fdout), 
    cell (a->cell), mtd (a->mtd) {}

static int
msg_recv (int fd, mtd_msg_t *msg)
{
  int rc;
  ssize_t sz = sizeof (mtd_msg_t);
  do {
    rc = read (fd, (void *)msg, sz);
    if (rc == sz)
      return rc;
    else if (rc > 0) {
      errno = EIO;
      return -1;
    }
  } while (rc < 0 && errno == EAGAIN);
  return rc;
}

static int
msg_send (int fd, mtd_msg_t *msg)
{
  char *buf = reinterpret_cast<char *> (msg);
  ssize_t sz = sizeof (mtd_msg_t);
  int bs = 0;
  int rc;

  while (bs < sz) {
    rc = write (fd, buf + bs, sz - bs);
    if (rc < 0 && errno != EAGAIN)
      return rc;
    bs += rc;
  }
  return sz;
}

mtdispatch_t::mtdispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s,
			    const txa_prog_t *x)
  : num (n), shmem (New mtd_shmem_t (n)), ntcb (c), maxq (m),
    nalive (n), sdflag (false), ssrv (s), txa_prog (x),
    quiet (false)
{}

mtdispatch_t::~mtdispatch_t ()
{
  warn << "in ~mtdispatch_t\n";
  delete shmem;
}

bool
mtdispatch_t::async_serv (svccb *b)
{
  u_int p = b->proc ();
  u_int la;
  switch (p) {
  case LOAD_AVG_RPC:
    la = ssrv ? ssrv->get_load_avg () : UINT_MAX;
    b->replyref (la);
    return true;
    break;
  case Q_LEN_RPC: 
    {
      u_int ql = queue.size ();
      b->replyref (ql);
      return true;
      break;
    }
  default:
    return false;
    break;
  }
}

bool
ssrv_client_t::authorized (u_int32_t procno)
{
  bool *b = authcache[procno];
  if (b) {
    return (*b);
  } else {
    bool ans = txa_prog->authorized (authtoks, procno);
    authcache.insert (procno, ans);
    return ans;
  }
}

void
mtdispatch_t::dispatch (svccb *b)
{
  if (ok_amt_stat_freq > 0)
    g_stats.report ();


  // if shutting down or there is a queue overflow, then
  // we need to reject the incoming request
  if (sdflag || queue.size () >= maxq) {
    warn << "XXX: rejecting / queue overflow\n"; // debug
    g_stats.rej ();
    b->reject (PROC_UNAVAIL);
  } 

  // this request might be immediately answerable by the async
  // wrapper class
  else if (async_serv (b)) {
    g_stats.async_serv ();
  } 

  // if there are elements in the queue, then this RPC needs
  // to go in the queue, also.  if there are no free worker
  // threads, then send_svccb will fail, and we need to 
  // queue
  else if (queue.size () > 0 || !send_svccb (b)) {
    g_stats.q ();
    if (!quiet) {
      warn << "Queuing (" << queue.size () << ")";
      size_t s = queue.size ();

      // this is a neat function to toggle reporting frequency
      // of which threads are actually working.  for low queue sizes,
      // it will say something every time.  for higher queue sizes,
      // it will try to conserve CPU and not 
      if (ok_amt_report_q_freq > 0 &&
	  (s < ok_amt_report_q_freq || (s % (s/ok_amt_report_q_freq) == 0)))
	warnx << ": " << which_procs ();
      warnx << "\n";
    }
    enqueue (b);
  } 

  // default case is that send_svccb returned true, and everything
  // is happy
  else {
    g_stats.in ();
  }

}

static void
which_procs_trav (strbuf *b, bool *f, const u_int &k, u_int *v)
{
  if (!*f)
    *b << ",";
  *f = false;
  *b << k << ":" << *v ;
}

str
mtdispatch_t::which_procs ()
{
  qhash<u_int, u_int> h;
  for (u_int i = 0; i < num; i++) {
    if (shmem->arr[i].status == MTD_WORKING && shmem->arr[i].sbp) {
      u_int proc = shmem->arr[i].sbp->proc ();
      u_int *n;
      if ((n = h[proc])) (*n) ++;
      else h.insert (proc, 0);
    }
  }
  strbuf b;
  bool first = true;
  b << "<";
  h.traverse (wrap (which_procs_trav, &b, &first));
  b << ">";
  return b;
}

bool
mtdispatch_t::send_svccb (svccb *b)
{
  if (readyq.size () > 0) {
    int i = readyq.pop_front ();
    assert (shmem->arr[i].status == MTD_READY);
    shmem->arr[i].status = MTD_WORKING;
    shmem->arr[i].sbp = b;
    if (msg_send (i, MTD_CONTINUE) < 0)
      warn ("mtdispatch::send_svccb: msg_send failed: %m\n");
    return true;
  } else {
    return false;
  }
}

int
mtdispatch_t::msg_send (int tid, mtd_status_t s)
{
  mtd_msg_t msg (tid, s);
  return ::msg_send (shmem->arr[tid].fdout, &msg);
}

void
mtdispatch_t::chld_msg ()
{
  mtd_msg_t msg;
  int rc = msg_recv (fdin, &msg);
  if (rc < 0) {
    warn ("Bad receive message: %m\n");
    return;
  }
  //warn << "Received message from child: " << msg << "\n"; // debug
  switch (msg.status) {
  case MTD_READY:
    if (sdflag) shutdown ();
    chld_ready (msg.tid);
    break;
  case MTD_REPLY:
    if (sdflag) shutdown ();
    chld_reply (msg.tid);
    break;
  case MTD_SHUTDOWN:
    warn << "Lost a child!\n";
    // XXX - recover?
    break;
  default:
    warn << "Unknown message received: " << msg << "\n";
  }
}

void
mtdispatch_t::shutdown ()
{
  sdflag = true;
  if (queue.size ())
    return;

  for (u_int i = 0; i < num; i++) 
    if (shmem->arr[i].status == MTD_WORKING)
      return;
  warn << "Shutdown complete.\n";
  exit (0);
}

void
mtdispatch_t::chld_reply (int i)
{
  mtd_shmem_cell_t *c = &(shmem->arr[i]);
  svccb *sbp = c->sbp;
  switch (c->rstat) {
  case MTD_RPC_NULL:
    ssrv->post_db_call (sbp, NULL);
    sbp->reply (NULL);
    break;
  case MTD_RPC_DATA:
    ssrv->post_db_call (sbp, c->rsp);
    sbp->reply (c->rsp);
    break;
  case MTD_RPC_REJECT:
    warn << "XXX: rejected by MTD_RPC_REJECT\n"; // DEBUG
    sbp->reject (PROC_UNAVAIL);
    break;
  }
  c->sbp = NULL;
  c->status = MTD_READY;
  c->rsp = NULL;
  chld_ready (i);
  g_stats.out ();
}

void
mtdispatch_t::chld_ready (int i)
{
  readyq.push_back (i);
  queue_el_t el;
  while (queue.size () && readyq.size ()) {
    el = queue.pop_front ();
    int diff = timenow - el.timein;
    if (diff > int (ok_amt_q_timeout)) {
      warn << "Timeout for object in queue (wait time=" 
	   << diff << "s)\n";
      el.sbp->reject (PROC_UNAVAIL);
      g_stats.to (); // report timeout to stats
    } else {
      send_svccb (el.sbp);
    }
  }
}

void 
mtdispatch_t::init ()
{
  int fds[2];
  if (pipe (fds) < 0)
    fatal << "mtdispatch::init: cannot open pipe\n";
  fdin = fds[0];
  make_async (fdin);
  fdcb (fdin, selread, wrap (this, &mtdispatch_t::chld_msg));
  for (u_int i = 0; i < num; i++) {
    launch (i, fds[1]);
  }
  //close (fds[1]);
}

static void
new_threadv (void *av)
{
  mtd_thread_arg_t *arg = static_cast<mtd_thread_arg_t *> (av);
  arg->mtd->new_thread (arg);
}

static void *
vnew_threadv (void *av)
{
  // warn << "vnew_threadv called.\n"; // debug
  new_threadv (av);
  return (NULL);
}

static int
inew_threadv (void *av)
{
  // warn << "inew_threadv called.\n";  // debug
  new_threadv (av);
  return 0;
}

//
// blah blah blah XXXX blah blah
void foo ()
{
  new_threadv (NULL);
  vnew_threadv (NULL);
  inew_threadv (NULL);
}

mtd_thread_t *
mtdispatch_t::new_thread (mtd_thread_arg_t *a) const
{
  //  warn << "New thread called: " << a->tid << "\n"; // debug
  mtd_thread_t *t = (*ntcb) (a);
  a->cell->thr = t;
  t->run (); 
  delete a;
  return t;
}

void
mtd_thread_t::run ()
{ 
  mtd_status_t rc;
  if (!init ()) {
    TWARN ("thread could not initialize");
    msg_send (MTD_SHUTDOWN);
    delete this;
    return;
  }

  become_ready ();
  do {
    take_svccb ();
    rc = msg_recv ();
  } while (rc == MTD_CONTINUE);
  
  cell->status = MTD_SHUTDOWN;
  msg_send (MTD_SHUTDOWN);
  return;
}

mtd_status_t 
mtd_thread_t::msg_recv ()
{
  mtd_msg_t msg;
  int rc = ::msg_recv (fdin, &msg);
  if (rc < 0) {
    warn ("mtd_thread_t::msg_recv: bad read: %m\n");
    return MTD_ERROR;
  }
  return msg.status;
}

int
mtd_thread_t::msg_send (mtd_status_t s)
{
  mtd_msg_t msg (tid, s);
  return ::msg_send (fdout, &msg);
}

void
mtd_thread_t::take_svccb ()
{
  svccb *s = cell->sbp;
  if (s) {
    start = timenow;
    dispatch (s);
  }
}

void
mtd_thread_t::become_ready ()
{
  readied = true;
  TWARN ("called become_ready ()"); // debug
  cell->status = MTD_READY;
  msg_send (MTD_READY);
}

void
mtd_thread_t::replynull ()
{
  did_reply ();
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_NULL;
  msg_send (MTD_REPLY);
}

void
mtd_thread_t::did_reply ()
{
  time_t tm = timenow - start;
  if (tm > LONG_REPLY_TIME) {
    if (cell->sbp) {
      TWARN ("long service time (" << tm << " secs) for PROC=" 
	     << cell->sbp->proc ());
    } else {
      TWARN ("long service time (" << tm  << " secs); no procno given");
    }
  }
}

void
mtd_thread_t::reject ()
{
  did_reply ();
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_REJECT;
  msg_send (MTD_REPLY);
}

void
mtd_thread_t::reply (ptr<void> d)
{
  did_reply ();
  cell->rsp = d;
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_DATA;
  msg_send (MTD_REPLY);
}

mtd_thread_arg_t *
mtdispatch_t::launch_init (int i, int fdout, int *closeit)
{
  int fds[2];
  if (pipe (fds) < 0)
    fatal << "mtdispatch::launch: cannot open pipe\n";
  make_async ((shmem->arr[i].fdout = fds[1]));
  mtd_thread_arg_t *arg = New mtd_thread_arg_t (i, fds[0], fdout, 
						&shmem->arr[i], this);
  *closeit = fds[0];
  return arg;
}

#ifdef HAVE_PTH
void
mgt_dispatch_t::init ()
{
  if (pth_init () != TRUE) {
    if (errno == EPERM)
      warn << "Pth library has already been initialized.\n";
    else
      fatal << "Cannot initialize GNU Pth library\n";
  }
  mtdispatch_t::init ();
}

void
mgt_dispatch_t::launch (int i, int fdout)
{
  // warn << "mgt_dispatch_t::launch: " << i << "\n"; // debug
  int closeit;
  mtd_thread_arg_t *arg = launch_init (i, fdout, &closeit);

  pth_attr_t attr = pth_attr_new ();
  names[i] = strbuf ("dispatch thread ") << i;
  pth_attr_set (attr, PTH_ATTR_NAME, names[i].cstr ());
  pth_attr_set (attr, PTH_ATTR_STACK_SIZE, MTD_STACKSIZE);
  pth_attr_set (attr, PTH_ATTR_JOINABLE, FALSE);
  if (!(gts[i] = pth_spawn (attr, vnew_threadv, static_cast<void *> (arg))))
    fatal << "mtdispatch::launch: pth_spawn failed\n";
}
#endif /* HAVE_PTH */

#ifdef HAVE_PTHREADS
void
mpt_dispatch_t::launch (int i, int fdout)
{
  // warn << "mpt_dispatch_t::launch: " << i << "\n"; // debug
  int closeit;
  mtd_thread_arg_t *arg = launch_init (i, fdout, &closeit);

  if (pthread_create (&pts[i], NULL, vnew_threadv, 
		      static_cast<void *> (arg)) < 0)
    fatal << "mtdispatch::launch: pthread_create failed\n";

  //close (closeit);
}
#endif /* HAVE_PTHREADS */

#ifdef HAVE_KTHREADS
void
mkt_dispatch_t::launch (int i, int fdout)
{
  // warn << "mkt_dispatch_t::launch: " << i << "\n"; // debug

  int closeit;
  mtd_thread_arg_t *arg = launch_init (i, fdout, &closeit);
#ifdef HAVE_RFORK_THREAD
  shmem->arr[i].stkp = (void *) xmalloc (MTD_STACKSIZE);
  int rc = rfork_thread (MTD_STACKSIZE, shmem->arr[i].stkp, inew_threadv,
			 static_cast<void *> (arg));
  if (rc < 0) {
    warn ("mtdispatch::launch: rfork failed: %m\n");
    exit (1);
  }
  shmem->arr[i].pid = rc;

#else
# ifdef HAVE_RFORK
  int rc = rfork (RFPROC);
  warn << "rfork returned: " << rc << "\n"; // debug
  if (rc == 0) {
    new_thread (arg);
    return;
  } else if (rc < 0) {
    warn ("mtdispatch::launch: rfork failed: %m\n");
    exit (1);
  }
# else
#  ifdef HAVE_CLONE
  shmem->arr[i].stkp = (void *) xmalloc (MTD_STACKSIZE);
  int rc = clone (inew_threadv, shmem->arr[i].stkp, 
		  CLONE_FS|CLONE_FILES|CLONE_VM,
		  static_cast<void *> (arg));
  if (rc < 0)
    fatal << "mtdispatch::launch: clone failed\n";
#else
  assert (false);
#endif
#endif
#endif
  //close (closeit);
}
#endif /* HAVE_KTHREADS */

void
ssrv_t::accept (ptr<axprt_stream> x)
{
  if (!x)
    fatal << "listen port closed.\n";
  vNew ssrv_client_t (this, prog, x, txa_prog);
}

ssrv_client_t::ssrv_client_t (ssrv_t *s, const rpc_program *const p, 
			      ptr<axprt> x, const txa_prog_t *t) 
  : ssrv (s), txa_prog (t)
{
  ssrv->insert (this);
  srv = asrv::alloc (x, *p, wrap (this, &ssrv_client_t::dispatch));
}

void
ssrv_client_t::dispatch (svccb *s)
{
  if (!s)
    delete this;
  else {
    u_int32_t procno = s->proc ();
    if (txa_prog && txa_prog->get_login_rpc () == procno) {
	txa_login_arg_t *arg = s->Xtmpl getarg<txa_login_arg_t> ();
	authtoks.clear ();
	for (u_int i = 0; i < arg->size (); i++) {
	  str tok ((*arg)[i].base (), (*arg)[i].size ());
	  authtoks.push_back (tok);
	}
	s->replyref (true);
    } else if (txa_prog && !authorized (procno)) {
      // XXX better debug message needed
      warn << "RPC rejected due to insufficient credentials!\n";
      s->reject (PROC_UNAVAIL);
    } else if (ssrv->skip_db_call (s)) {
      ssrv->mtd->g_stats.async_serv ();
    } else {
      ssrv->req_made ();
      ssrv->mtd->dispatch (s);
    }
  }
}

ssrv_client_t::~ssrv_client_t () { ssrv->remove (this); }

ssrv_t::ssrv_t (newthrcb_t c, const rpc_program &p, 
		mtd_thread_typ_t typ, int n, int m, const txa_prog_t *x) 
  : mtd (NULL), prog (&p), load_avg (0), txa_prog (x)
{
  switch (typ) {
  case MTD_KTHREADS:
#ifdef HAVE_KTHREADS 
    mtd = New mkt_dispatch_t (c, n, m, this, x);
#else
    warn << "kthreads are not available with this build!\n";
#endif /* HAVE_KTHREADS */
    break;
  case MTD_PTH:
#ifdef HAVE_PTH
    assert (PTH_SYSCALL_HARD && ! PTH_SYSCALL_SOFT);
    mtd = New mgt_dispatch_t (c, n, m, this, x);
#else
    warn << "pth is not available with this build!\n";
#endif /* HAVE_PTH */
    break;
  case MTD_PTHREADS:
#ifdef HAVE_PTHREADS
    mtd = New mpt_dispatch_t (c, n, m, this, x);
#else 
    warn << "pthreads are not available with this build!\n";
#endif /* HAVE_PTHREADS */
    break;
  default:
    break;
  }
  if (!mtd) {
#ifdef HAVE_PTH
    warn << "Requested thread type not availabe: using GNU Pth\n";
    mtd = New mgt_dispatch_t (c, n, m, this, x);
#else
# ifdef HAVE_PTHREADS
    warn << "Requested thread type not availabe: using POSIX pthreads\n";
    mtd = New mpt_dispatch_t (c, n, m, this, x);
# else
#  ifdef HAVE_KTHREADS 
    warn << "Requested thread type not availabe: using kernel threads\n";
    mtd = New mkt_dispatch_t (c, n, m, this, x);
#  else
    panic ("No threading package available!\n");
#  endif  /* HAVE_KTHREADS */
# endif   /* HAVE_PTHREADS */
#endif    /* HAVE_PTH */
  }
  mtd->init (); 
}

bool
tsdiff (const struct timespec &ts1, const struct timespec &ts2, int diff)
{
  long sd = ts2.tv_sec - ts1.tv_sec;
  return (sd > diff || (sd == diff && ts2.tv_nsec > ts1.tv_nsec));
}

void
mtd_stats_t::report ()
{
  if (ok_amt_stat_freq == 0 ||
      !tsdiff (start_sample, tsnow, ok_amt_stat_freq))
    return;
  epoch_t e = new_epoch ();
  warn ("STATS: i=%d;o=%d;r=%d;q=%d;a=%d;t=%d;l=%d\n",
	e.in, e.out, e.rejects, e.queued, e.async_serv, e.to, e.len_msec);
}

void
ssrv_t::req_made ()
{
  reqtimes.push_back (tsnow);
  while (reqtimes.size () && tsdiff (reqtimes[0], tsnow, ok_amt_lasi))
    reqtimes.pop_front ();
  load_avg = reqtimes.size ();
}
       
ssrv_t *
mtd_thread_t::get_ssrv ()
{
  return mtd->get_ssrv ();
}

ssrv_t *
mtd_thread_t::get_ssrv () const
{
  return mtd->get_ssrv ();
}

epoch_t
mtd_stats_t::new_epoch ()
{
  epoch_t e = sample;

  e.len_msec = (tsnow.tv_sec - start_sample.tv_sec) * 1000
    + (tsnow.tv_nsec - start_sample.tv_nsec) / 1000000;

  start_sample = tsnow;
  sample.in = 0;
  sample.out = 0;
  sample.rejects = 0;
  sample.queued = 0;
  sample.async_serv = 0;
  sample.to = 0;

  return e;
}
