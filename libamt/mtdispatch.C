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
#include "rxx.h"
#include "parseopt.h"
#include "rpc_stats.h"
#ifdef HAVE_PTHREADS
#include "amt_pthread.h"
#endif

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

mtdispatch_t::mtdispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s)
  : num (n), shmem (New mtd_shmem_t (n)), ntcb (c), maxq (m),
    nalive (n), sdflag (false), ssrv (s), quiet (false)
{}

mtdispatch_t::~mtdispatch_t ()
{
  warn << "in ~mtdispatch_t\n";
  delete shmem;
}

void mtdispatch_t::set_max_q (u_int32_t q)
{ maxq = min<u_int> (q, MTD_MAXQ); }

bool
mtdispatch_t::async_serv (svccb *b)
{
  return false;
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
      else h.insert (proc, 1);
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
    ssrv->post_db_call (sbp, c->rsp_u.p);
    sbp->reply (c->rsp_u.p);
    c->rsp_u.p = NULL;
    break;

  case MTD_RPC_BOOL:
    sbp->replyref (c->rsp_u.b);
    break;

  case MTD_RPC_UINT32:
    sbp->replyref (c->rsp_u.u32);
    break;

  case MTD_RPC_INT32:
    sbp->replyref (c->rsp_u.i32);
    break;

  case MTD_RPC_PASSPTR:
    sbp->reply (c->rsp_u.pp.obj ());
    c->rsp_u.pp.clear ();
    break;

#ifdef HAVE_BOOST_SHARED_PTR
  case MTD_RPC_BOOST_PTR:
    sbp->reply (c->rsp_u.bp.get ());
    c->rsp_u.bp.reset ();
    break;
#endif /* HAVE_BOOST_SHARED_PTR */

  case MTD_RPC_REJECT:
    warn << "XXX: rejected by MTD_RPC_REJECT\n"; // DEBUG
    sbp->reject (c->err_code);
    break;

  default:
    warn << "XXX: unhandled case!\n";
    sbp->reject (PROC_UNAVAIL);
    break;
  }

  c->sbp = NULL;
  c->status = MTD_READY;
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
    int diff = sfs_get_timenow() - el.timein;
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
mtdispatch_t::init_rpc_stats ()
{
  if (ok_amt_rpc_stats)
    get_rpc_stats().set_active(true).set_interval (ok_amt_rpc_stats_interval);
}

void 
mtdispatch_t::init ()
{

  init_rpc_stats ();

  int fds[2];

  if (num == 0) {
    warn << "Cannot start program with 0 threads; exiting...\n";
    exit (3);
  }

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

void
amt_new_threadv (void *av)
{
  mtd_thread_arg_t *arg = static_cast<mtd_thread_arg_t *> (av);
  arg->mtd->new_thread (arg);
}

void *
amt_vnew_threadv (void *av)
{
  // warn << "vnew_threadv called.\n"; // debug
  amt_new_threadv (av);
  return (NULL);
}

mtd_thread_t *
mtdispatch_t::new_thread (mtd_thread_arg_t *a) const
{
  //  warn << "New thread called: " << a->tid << "\n"; // debug
  mtd_thread_t *t = (*ntcb) (a);
  a->cell->thr = t;
  const bool ok = t->run ();
  delete a;
  if (!ok) {
    delete t;
    return nullptr;
  }
  return t;
}

bool
mtd_thread_t::run ()
{ 
  mtd_status_t rc;

  GIANT_LOCK();
  bool ok = init_phase0() && init();
  GIANT_UNLOCK();

  if (!ok) {
    TWARN ("thread could not initialize");
    msg_send (MTD_SHUTDOWN);
    return false;
  }

  become_ready ();
  do {
    GIANT_LOCK();
    take_svccb ();
    GIANT_UNLOCK();
    rc = msg_recv ();
  } while (rc == MTD_CONTINUE);
  
  cell->status = MTD_SHUTDOWN;
  msg_send (MTD_SHUTDOWN);
  return true;
}

mtd_status_t 
mtd_thread_t::msg_recv ()
{
  mtd_msg_t msg;
  int rc = ::msg_recv (fdin, &msg);
  if (rc < 0) {
    TWARN ("mtd_thread_t::msg_recv: bad read; (errno=%d" << errno << ")");
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
    start = sfs_get_timenow();
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
  time_t tm = sfs_get_timenow() - start;
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
mtd_thread_t::reject (enum accept_stat err)
{
  did_reply ();
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_REJECT;
  cell->err_code = err;
  msg_send (MTD_REPLY);
}

void
mtd_thread_t::reply_b (bool b)
{
  did_reply ();
  cell->rsp_u.b = b;
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_BOOL;
  msg_send (MTD_REPLY);
}

void
mtd_thread_t::reply_i32 (int32_t i)
{
  did_reply ();
  cell->rsp_u.i32 = i;
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_INT32;
  msg_send (MTD_REPLY);
}

void
mtd_thread_t::reply_u32 (u_int32_t u)
{
  did_reply ();
  cell->rsp_u.u32 = u;
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_UINT32;
  msg_send (MTD_REPLY);
}

void
mtd_thread_t::reply (ptr<void> d)
{
  if (ok_kthread_safe) {
    TWARN ("XX WARNING!! Do not call mth_thread_t::reply "
	   "in thread-safe mode!\n");
  }
  did_reply ();
  cell->rsp_u.p = d;
  d = NULL;
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_DATA;
  msg_send (MTD_REPLY);
}

#ifdef HAVE_BOOST_SHARED_PTR
void
mtd_thread_t::reply (boost::shared_ptr<void> p)
{
  did_reply ();
  cell->rsp_u.bp = p;
  p.reset ();
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_BOOST_PSTR;
  msg_send (MTD_REPLY);
}
#endif /* HAVE_BOOST_SHARED_PTR */

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
  if (!(gts[i] = pth_spawn (attr, amt_vnew_threadv, static_cast<void *> (arg))))
    fatal << "mtdispatch::launch: pth_spawn failed\n";
}
#endif /* HAVE_PTH */


void
ssrv_t::accept (ptr<axprt_stream> x)
{
  if (!x)
    fatal << "listen port closed.\n";
  vNew ssrv_client_t (this, prog, x);
}

ssrv_client_t::ssrv_client_t (ssrv_t *s, const rpc_program *const p, 
			      ptr<axprt_stream> x)
  : ssrv (s), _x (x)
{
  ssrv->insert (this);
  srv = asrv::alloc (x, *p, wrap (this, &ssrv_client_t::dispatch));
  init_reporting_info ();
}

void
ssrv_client_t::init_reporting_info ()
{
  int fd = _x->getreadfd ();
  sockaddr_in sin;
  socklen_t sl = sizeof (sin);
  memset (&sin, 0, sizeof (sin));

  const char *a = "<getpeername failure>";
  _port = 0;

  if (getpeername (fd, (sockaddr *)&sin, &sl) == 0) {
    a = inet_ntoa (sin.sin_addr);
    if (!a) a = "<inet_ntoa failure>";
    _port = ntohs (sin.sin_port);
  }

  _hostname = a;
}

void
ssrv_client_t::dispatch (svccb *s)
{
  if (!s)
    delete this;
  else {
    u_int32_t procno = s->proc ();
    ssrv->mtd->g_reporting.rpc_report (procno, _hostname, _port);
    if (ssrv->skip_db_call (s)) {
      ssrv->mtd->g_stats.async_serv ();
    } else {
      ssrv->req_made ();
      ssrv->mtd->dispatch (s);
    }
  }
}

ssrv_client_t::~ssrv_client_t () { ssrv->remove (this); }

ssrv_t::ssrv_t (const rpc_program &p)
  : prog (&p), load_avg (0) {}

void
ssrv_t::init (mtdispatch_t *m)
{
  mtd = m;
  mtd->init ();
}

mtdispatch_t *g_mtdispatch;

ssrv_t::ssrv_t (newthrcb_t c, const rpc_program &p, 
		mtd_thread_typ_t typ, int n, int m)
  : mtd (NULL), prog (&p), load_avg (0)
{

  bool ok = false;

  if (typ == MTD_PTH) { 
#ifdef HAVE_PTH
    assert (PTH_SYSCALL_HARD && ! PTH_SYSCALL_SOFT);
    mtd = New mgt_dispatch_t (c, n, m, this);
    mtd->init (); 
    ok = true;
#else /* HAVE_PTH */
    panic ("pth is not available with this build; "
	   "cannot continue without threads\n");
#endif
  }
  
  if (typ == MTD_PTHREAD) {
#ifdef HAVE_PTHREADS
    mtd = New mpt_dispatch_t (c, n, m, this);
    mtd->init (); 
    ok = true;
#else
    panic ("pthreads is not available; try --enable-pthreads");
#endif
  }

  if (!ok) {
    panic ("no threading package available!");
  }

  // Keep a global pointer to it, so that we can 
  g_mtdispatch = mtd;
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
      !tsdiff (start_sample, sfs_get_tsnow(), ok_amt_stat_freq))
    return;
  epoch_t e = new_epoch ();
  warn ("STATS: i=%d;o=%d;r=%d;q=%d;a=%d;t=%d;l=%d\n",
	e.in, e.out, e.rejects, e.queued, e.async_serv, e.to, e.len_msec);
}

void
ssrv_t::req_made ()
{
  reqtimes.push_back (sfs_get_tsnow());
  while (reqtimes.size () && 
	 tsdiff (reqtimes[0], sfs_get_tsnow(), ok_amt_lasi))
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
  
  struct timespec tsnow;
  tsnow = sfs_get_tsnow ();

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

bool
mtd_reporting_t::set_rpc_reports (const str &s)
{
  bool ok = true;
  static rxx x ("\\s*,\\s*");
  _rpcs.clear ();
  if (s) {
    vec<str> v;
    split (&v, x, s);
    for (size_t i = 0; i < v.size (); i++) {
      int tmp;
      if (convertint (v[i], &tmp)) {
	_rpcs.insert (tmp);
      } else {
	ok = false;
	warn << "Bad RPC number specified for reporting: " << v[i] << "\n";
      }
    }
  }
  return ok;
}

void
mtd_reporting_t::rpc_report (int rpc, const str &s, int p)
{
  if (_rpcs[rpc]) {
    warn << "RPC=" << rpc << "; hostname=" << s << "; port=" << p << "\n";
  }
}

mtd_reporting_t::mtd_reporting_t ()
{
  const char *p = safegetenv ("DBPRX_REPORT_RPCS");
  if (p) set_rpc_reports (p);
}

//=======================================================================

namespace amt {

  //-----------------------------------------------------------------------

  void
  thread2_t::dispatch (svccb *sbp)
  {
    ptr<req_t> rq = New refcounted<req_t> (this, sbp);
    dispatch (rq);
  }

  //-----------------------------------------------------------------------

  void req_t::replynull () { _thr->replynull (); }
  void req_t::reject (enum accept_stat as) { _thr->reject (as); }
  void req_t::reply (ptr<void> d) { _thr->reply (d); }
  void req_t::reply_b (bool b) { _thr->reply_b (b); }
  void req_t::reply_i32 (int32_t i) { _thr->reply_i32 (i); }
  void req_t::reply_u32 (u_int32_t i) { _thr->reply_u32 (i); }
  const void *req_t::getvoidarg () const { return _sbp->getvoidarg (); }
  void *req_t::getvoidarg () { return _sbp->getvoidarg (); }

  //-----------------------------------------------------------------------

};


//=======================================================================
