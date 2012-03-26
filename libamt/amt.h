// -*-c++-*-
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

//
// AMT = Asynchronous Multi-Threading
//
//   A generic adaptor framework for interface async-style programs
//   with a synchronous-style library, like libmysqlclient.
//

#ifndef _LIBAMT_AMT_H
#define _LIBAMT_AMT_H

#include "okwsconf.h"
#include "litetime.h"
#include "pubutil.h"
#include "passptr.h"

#ifdef HAVE_BOOST_SHARED_PTR
# include  <boost/shared_ptr.hpp>
#endif

#ifdef HAVE_PTH

//
// Pth Requires an FD_SETSIZE < 1024
//
#ifdef FD_SETSIZE
# if FD_SETSIZE > 1024
#  undef FD_SETSIZE
#  define FD_SETSIZE 1024
# endif
#endif

#include <pth.h>
#endif /* HAVE_PTH */

#include "okconst.h"
#include "arpc.h"
#include <sys/time.h>

#define MTD_NTHREADS    5
#define MTD_MAXQ        1000
#define MTD_STACKSIZE   0x100000

#define TWARN(x)						\
  do {								\
    strbuf b;							\
    b << progname << "[" << getid() << "]: " << x << "\n";	\
    str s = b;							\
    fix_trailing_newlines (&s);					\
    fprintf (stderr, "%s", s.cstr ());				\
  } while (0)

typedef enum { MTD_NONE = 0, MTD_PTH = 1, MTD_PTHREAD } mtd_thread_typ_t;

// MTD = Mutli-Thread Dispatcher

class mtdispatch_t;
class mtd_shmem_cell_t;

struct mtd_thread_arg_t {
  mtd_thread_arg_t (int i, int in, int o, mtd_shmem_cell_t *c, mtdispatch_t *d)
   : tid (i), fdin (in), fdout (o), cell (c), mtd (d) {}

  int tid;
  int fdin;
  int fdout;
  mtd_shmem_cell_t *cell;
  mtdispatch_t *mtd;
};

typedef enum { MTD_STARTUP = 0, MTD_READY = 1, 
	       MTD_WORKING = 2, MTD_SHUTDOWN = 3, 
	       MTD_ERROR = 4, MTD_CONTINUE = 5,
               MTD_REPLY = 6 } mtd_status_t;

typedef enum { MTD_RPC_NULL = 0,
	       MTD_RPC_DATA = 1,
	       MTD_RPC_REJECT = 2,
	       MTD_RPC_BOOL = 3 ,
	       MTD_RPC_INT32 = 4,
	       MTD_RPC_UINT32 = 5, 
	       MTD_RPC_PASSPTR = 6,
	       MTD_RPC_BOOST_PTR = 7 } mtd_rpc_t;

struct epoch_t {
  epoch_t () : len_msec (0) {}
  int in;
  int out;
  int rejects;
  int queued;
  int async_serv;
  int len_msec;
  int to; // time outs!
};

struct mtd_stats_t {
  mtd_stats_t () : start_sample (sfs_get_tsnow ()) {}

  inline void in () { sample.in ++ ; total.in ++; }
  inline void out () { sample.out ++ ; total.out ++; }
  inline void rej () { sample.rejects ++ ; total.rejects ++; }
  inline void q () { sample.queued ++ ; total.queued ++; }
  inline void async_serv () { sample.async_serv ++; total.async_serv ++; }
  inline void to () { sample.to ++; total.to ++; }

  epoch_t new_epoch ();
  epoch_t get_total () { return total; }
  void report ();

  struct timespec start_sample;
  epoch_t sample;
  epoch_t total;
};

struct mtd_reporting_t {
public:
  mtd_reporting_t ();
  void rpc_report (int rpc, const str &s, int port);
  bool set_rpc_reports (const str &s);
private:
  bhash<int> _rpcs;
};

class ssrv_t;
class mtd_thread_t { // Abstract class for Child threads
public:
  mtd_thread_t (mtd_thread_arg_t *a);
  virtual ~mtd_thread_t () {}
  void run ();
  virtual void dispatch (svccb *sbp) = 0;
  virtual bool init () { return true; }
  virtual bool init_phase0 () { return true; }
  void finish ();

  void replynull ();
  void reject (enum accept_stat as = PROC_UNAVAIL);

  void reply (ptr<void> d);
  void reply_b (bool b);
  void reply_i32 (int32_t i);
  void reply_u32 (u_int32_t u);

#ifdef HAVE_BOOST_SHARED_PTR
  void reply_boost (shared_ptr<T> p);
#endif

  template<class T> void reply_pass (passptr<T> p);

  int getid () const { return tid; }
  ssrv_t *get_ssrv ();
  ssrv_t *get_ssrv () const;
private:
  void become_ready ();
  void take_svccb ();
  int msg_send (mtd_status_t s);
  mtd_status_t msg_recv ();
protected:
  virtual void did_reply () ;
  int tid;
  bool readied;
private:
  time_t start;
  int fdin;
  int fdout;
  mtd_shmem_cell_t *cell;
  mtdispatch_t *mtd;
};

//=======================================================================

//
// For Versions 2.0.6pre5 and forward, I'm going to try to put as much
// code as possible here in the AMT namespace.  For now, just start
// with the new-style of threads, which are better about hiding the
// parts of the svccb object that should not be used by threads.
//
namespace amt {

  class req_t {
  public:
    req_t (mtd_thread_t *thr, svccb *sbp) : _thr (thr), _sbp (sbp) {}
    
    void replynull ();
    void reject (enum accept_stat as = PROC_UNAVAIL);
    void reply (ptr<void> d);
    void reply_b (bool b);
    void reply_i32 (int32_t i);
    void reply_u32 (u_int32_t u);

    template<class T> void replyref (const T &res)
    {
      ptr<T> arg = New refcounted<T>(res);
      reply (arg);
    }
    void replyref (const bool res)
    { reply_b (res); }
    void replyref (const int32_t res)
    { reply_i32 (res); }
    void replyref (const u_int32_t res)
    { reply_u32 (res); }


    const void *getvoidarg () const;
    void *getvoidarg ();

    u_int32_t xid () const { return _sbp->xid (); }
    u_int32_t prog () const { return _sbp->prog (); }
    u_int32_t vers () const { return _sbp->vers (); }
    u_int32_t proc () const { return _sbp->proc (); }

    template<class T> T *
    getarg () { return static_cast<T *> (getvoidarg ()); }
    template<class T> const T *
    getarg () const { return static_cast<T *> (getvoidarg ()); }
  private:
    mtd_thread_t *_thr;
    svccb *_sbp;
  };

  // This new thread class deals in req_t's, which hide the svccb
  // and expose a tidier interface to the programmer.
  class thread2_t : public mtd_thread_t {
  public:
    thread2_t (mtd_thread_arg_t *a) : mtd_thread_t (a) {}
    virtual ~thread2_t () {}
    virtual void dispatch (ptr<req_t> req) = 0;
  private:
    void dispatch (svccb *sbp);
  };
};

//=======================================================================

typedef callback<mtd_thread_t *, mtd_thread_arg_t *>::ref newthrcb_t;

struct mtd_msg_t {
  mtd_msg_t (int i, mtd_status_t s) : tid (i), status (s) {}
  mtd_msg_t () : tid (0), status (MTD_STARTUP) {}
  str to_str () const 
  { return strbuf("Thread Id: ") << tid << "; Status: " << int (status); }
  operator str() const { return to_str (); }
  int tid;
  mtd_status_t status;
};

struct mtd_shmem_cell_t { // Used to communicate between Child and Dispatch
  mtd_shmem_cell_t () : status (MTD_STARTUP), sbp (NULL), thr (NULL), 
			fdout (-1), stkp (NULL), pid (0) {}
  ~mtd_shmem_cell_t () 
  { 
    if (sbp) sbp->reject (PROC_UNAVAIL); 
    if (stkp) xfree (stkp); 
  }
  mtd_status_t status; // communicate by setting this bit
  svccb *sbp;          // dispatch puts incoming req here
  mtd_thread_t *thr;   // for dispatch to delete thread ??
  int fdout;           // for dispatch to send messages out (pipe)
  void *stkp;          // on linux, top of stack for kernel thread
  int pid;             // for kernel threads, the PID of the thread

  mtd_rpc_t rstat;     // response status
  enum accept_stat err_code; // if rejecting, use this err code.

  // Should be a union, but make it a struct since we have a lot of memory,
  // and there's an issue with C++ objects inside a union.
  struct {
    ptr<void> p;       // response data
    bool b;            // boolean response
    int32_t i32;       // int32 response
    u_int32_t u32;     // u_int32 respoonse
    passptr<void> pp;  // pass-ptr response
#ifdef HAVE_BOOST_SHARED_PTR
    boost::shared_ptr<void> bp; // boost shared pointer
#endif
  } rsp_u;             // response is one of the above (or NULL)

};

struct mtd_shmem_t {
  mtd_shmem_t (int n) : arr (New mtd_shmem_cell_t [n]) {}
  ~mtd_shmem_t () { delete [] arr; } 
  mtd_shmem_cell_t *arr;
};

struct queue_el_t {
  queue_el_t () {}
  queue_el_t (svccb *s, time_t t) : sbp (s), timein (t) {}
  svccb *sbp;
  time_t timein;
};

class ssrv_client_t;
class mtdispatch_t { // Multi-Thread Dispatch
public:
  mtdispatch_t (newthrcb_t c, u_int nthr, u_int mq, ssrv_t *s);
  void dispatch (svccb *b);
  mtd_thread_t *new_thread (mtd_thread_arg_t *a) const;
  virtual ~mtdispatch_t ();
  virtual void init ();
  virtual bool async_serv (svccb *b);
  ssrv_t *get_ssrv () { return ssrv; }
  ssrv_t *get_ssrv () const { return ssrv; }
  str which_procs (); // debugging function
  void set_quiet (bool q) { quiet = q; }

  virtual void giant_lock () {}
  virtual void giant_unlock () {}

  template<class T> void thread_apply (typename callback<void, T *>::ref cb) 
  {
    for (u_int i = 0; i < num; i++) {
      (*cb) (dynamic_cast<T *> (shmem->arr[i].thr));
    }
  }

  void enqueue (svccb *b) 
  {
    queue.push_back (queue_el_t (b, sfs_get_timenow()));
  }

  void set_max_q (u_int32_t q);
  u_int32_t get_max_q () const { return maxq; }
  
protected:
  mtd_thread_arg_t *launch_init (int i, int fdout, int *closeit);
  virtual void launch (int i, int fd) = 0;
  virtual void init_rpc_stats ();
  
  void shutdown ();
  int msg_send (int tid, mtd_status_t s);
  bool send_svccb (svccb *b);
  void chld_reply (int i);
  void chld_ready (int i);
  void chld_msg ();

  u_int num;
  mtd_shmem_t *shmem;      // (mostly) shared memory
  int fdin;                // pipe coming in
  newthrcb_t ntcb;         // new thread cb
  u_int maxq;              // max q size
  int nalive;              // number of children alive
  bool sdflag;             // shutdown flag
  
  vec<queue_el_t> queue;      // queue of waiting connections
  vec<int> readyq;         // ready threads
  ssrv_t *ssrv;            // synchronous server pointer


public:
  mtd_stats_t  g_stats;         // global stats
  mtd_reporting_t g_reporting;  // Report about who is making these rpcs

protected:
  bool quiet;              // on if should make no noise
};

class ssrv_client_t {
public:
  ssrv_client_t (ssrv_t *s, const rpc_program *const p, ptr<axprt_stream> x);
  ~ssrv_client_t ();
  void dispatch (svccb *s);
  list_entry<ssrv_client_t> lnk;
protected:
  void init_reporting_info ();
private:
  ssrv_t *ssrv;
  ptr<asrv> srv;
  ptr<axprt_stream> _x;

  // hostname/port of the guy on the other end
  str _hostname;
  int _port;
};

extern mtdispatch_t *g_mtdispatch;

#define GIANT_LOCK()				\
  do {						\
    if (g_mtdispatch) {				\
      g_mtdispatch->giant_lock ();		\
    }						\
  } while (0)

#define GIANT_UNLOCK()				\
  do {						\
    if (g_mtdispatch) {				\
      g_mtdispatch->giant_unlock ();		\
    }						\
  } while (0)


class ssrv_t { // Synchronous Server (I.e. its threads can block)
public:
  ssrv_t (newthrcb_t c, const rpc_program &p, mtd_thread_typ_t typ = MTD_PTH, 
	  int nthr = MTD_NTHREADS, int mq = MTD_MAXQ);

  // use these two to pass in your own virtual mtdispatch object.
  ssrv_t (const rpc_program &p);
  void init (mtdispatch_t *m);

  void accept (ptr<axprt_stream> x);
  void insert (ssrv_client_t *c) { lst.insert_head (c); }
  void remove (ssrv_client_t *c) { lst.remove (c); }
  virtual bool skip_db_call (svccb *c) { return false; }
  virtual void post_db_call (svccb *c, ptr<void> resp) {}
  virtual ~ssrv_t () { warn << "in ~ssrv_t()\n"; delete mtd; }
  void req_made (); // called for accounting purposes
  u_int get_load_avg () const { return load_avg; }

  void set_max_q (u_int32_t q) { mtd->set_max_q (q); }
  u_int32_t get_max_q () const { return mtd->get_max_q (); }

  template<class T> void thread_apply (typename callback<void, T *>::ref cb)
  { mtd->thread_apply<T> (cb); }

  void giant_lock () { mtd->giant_lock (); }
  void giant_unlock () { mtd->giant_unlock (); }

  mtdispatch_t *mtd;
private:
  const rpc_program *const prog;
  list<ssrv_client_t, &ssrv_client_t::lnk> lst;
  vec<struct timespec> reqtimes;
  u_int load_avg;
};

#ifdef HAVE_PTH
class mgt_dispatch_t : public mtdispatch_t  // Pth Threads
{
public:
  mgt_dispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s) :
    mtdispatch_t (c, n, m, s), names (New str [n]), gts (New pth_t [n]) {}
  ~mgt_dispatch_t () 
  { 
    warn << "in ~mgt_dispatch_t\n"; 
    delete [] names; 
    delete [] gts; 
  }
  void init ();
  void launch (int i, int fdout);
protected:
  str *names;
  pth_t *gts;
};
#endif /* HAVE_PTH */

bool
tsdiff (const struct timespec &ts1, const struct timespec &ts2, int diff);

void *amt_vnew_threadv (void *av);
void amt_new_threadv (void *av);


template<class T> void
mtd_thread_t::reply_pass (passptr<T> p)
{
  did_reply ();
  p.pass_to (cell->rsp_u.pp);
  cell->status = MTD_REPLY;
  cell->rstat = MTD_RPC_PASSPTR;
  msg_send (MTD_REPLY);
}


#endif /* _LIBAMT_AMT_H */
