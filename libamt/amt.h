
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAMT_AMT_H
#define _LIBAMT_AMT_H

#include "okwsconf.h"
#include "lbalance.h"

#ifdef HAVE_CLONE
# include <sched.h>
#else
# ifdef HAVE_RFORK
#  include <unistd.h>
#  include <sys/param.h>
# endif /* HAVE_RFORK */
#endif /* HAVE_CLONE */

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif /* HAVE_PTHREAD */

#ifdef HAVE_PTH
#include <pth.h>
#endif /* HAVE_PTH */

#include "okconst.h"
#include "arpc.h"
#include <sys/time.h>

#define MTD_NTHREADS    5
#define MTD_MAXQ        1000
#define MTD_STACKSIZE   0x100000


#define TWARN(x) \
  warn << "thread " << tid << ": " << x << "\n"

typedef enum { MTD_KTHREADS = 0, MTD_PTH = 1, 
	       MTD_PTHREADS = 3 } mtd_thread_typ_t;

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
	       MTD_RPC_REJECT = 2 } mtd_rpc_t;

class mtd_thread_t { // Abstract class for Child threads
public:
  mtd_thread_t (mtd_thread_arg_t *a);
  void run ();
  virtual void dispatch (svccb *sbp) = 0;
  virtual bool init () { return true; }
  void finish ();

  void replynull ();
  void reject ();
  void reply (ptr<void> d);
  int getid () const { return tid; }
private:
  void become_ready ();
  void take_svccb ();
  int msg_send (mtd_status_t s);
  mtd_status_t msg_recv ();
protected:
  int tid;
private:
  int fdin;
  int fdout;
  mtd_shmem_cell_t *cell;
  mtdispatch_t *mtd;
};

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
  ptr<void> rsp;       // response data
};

struct mtd_shmem_t {
  mtd_shmem_t (int n) : arr (New mtd_shmem_cell_t [n]) {}
  ~mtd_shmem_t () { delete [] arr; } 
  mtd_shmem_cell_t *arr;
};

class ssrv_t;
class mtdispatch_t { // Multi-Thread Dispatch
public:
  mtdispatch_t (newthrcb_t c, u_int nthr, u_int mq, ssrv_t *s);
  void dispatch (svccb *b);
  mtd_thread_t *new_thread (mtd_thread_arg_t *a) const;
  virtual ~mtdispatch_t ();
  virtual void init ();
  virtual bool async_serv (svccb *b);

protected:
  mtd_thread_arg_t *launch_init (int i, int fdout, int *closeit);
  virtual void launch (int i, int fd) = 0;
  
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
  
  vec<svccb *> queue;      // queue of waiting connections
  vec<int> readyq;         // ready threads
  ssrv_t *ssrv;            // synchronous server pointer
};

class ssrv_client_t {
public:
  ssrv_client_t (ssrv_t *s, const rpc_program *const p, ptr<axprt> x);
  ~ssrv_client_t ();
  void dispatch (svccb *s);
  list_entry<ssrv_client_t> lnk;
private:
  ssrv_t *ssrv;
  ptr<asrv> srv;
};

class ssrv_t { // Synchronous Server (I.e. its threads can block)
public:
  ssrv_t (newthrcb_t c, const rpc_program &p, mtd_thread_typ_t typ = MTD_PTH, 
	  int nthr = MTD_NTHREADS, int mq = MTD_MAXQ);
  void accept (ptr<axprt_stream> x);
  void insert (ssrv_client_t *c) { lst.insert_head (c); }
  void remove (ssrv_client_t *c) { lst.remove (c); }
  virtual bool skip_db_call (svccb *c) { return false; }
  virtual void post_db_call (svccb *c, ptr<void> resp) {}
  virtual ~ssrv_t () { delete mtd; }
  void req_made (); // called for accounting purposes
  u_int get_load_avg () const { return load_avg; }

  mtdispatch_t *mtd;
private:
  const rpc_program *const prog;
  list<ssrv_client_t, &ssrv_client_t::lnk> lst;
  vec<struct timespec> reqtimes;
  u_int load_avg;
};

#ifdef HAVE_KTHREADS
class mkt_dispatch_t : public mtdispatch_t  // Kernel Threads
{
public:
  mkt_dispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s) 
    : mtdispatch_t (c, n, m, s) {}
  void launch (int i, int fdout);
};
#endif /* HAVE_KTHREADS */

#ifdef HAVE_PTHREADS
class mpt_dispatch_t : public mtdispatch_t // Posix Threads
{
public:
  mpt_dispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s) :
    mtdispatch_t (c, n, m, s), pts (New pthread_t [n]) {}

  ~mpt_dispatch_t () { delete [] pts; } 
  void launch (int i, int fdout);
protected:
  pthread_t *pts;
};
#endif /* HAVE_PTHREADS */

#ifdef HAVE_PTH
class mgt_dispatch_t : public mtdispatch_t  // Pth Threads
{
public:
  mgt_dispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s) :
    mtdispatch_t (c, n, m, s), names (New str [n]), gts (New pth_t [n]) {}
  ~mgt_dispatch_t () { delete [] names; delete [] gts; }
  void init ();
  void launch (int i, int fdout);
protected:
  str *names;
  pth_t *gts;
};
#endif /* HAVE_PTH */

#endif /* _LIBAMT_AMT_H */
