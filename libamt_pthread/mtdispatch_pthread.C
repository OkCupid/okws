
#include "amt_pthread.h"

mpt_dispatch_t::mpt_dispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s,
				const txa_prog_t *x)
 : mtdispatch_t (c, n, m, s, x), 
   pts (New pthread_t [n])
{
  int rc = pthread_mutex_init (&_giant_lock, NULL);
  if (rc != 0) {
    fatal ("pthread_mutex_init() failed (%m)\n");
  }
}

void
mpt_dispatch_t::giant_lock ()
{
  pthread_mutex_lock (&_giant_lock);
}

void
mpt_dispatch_t::giant_unlock ()
{
  pthread_mutex_unlock (&_giant_lock);
}

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

