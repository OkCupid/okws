
#include "amt_pthread.h"

mpt_dispatch_t::mpt_dispatch_t (newthrcb_t c, u_int n, u_int m, ssrv_t *s)
  : mtdispatch_t (c, n, m, s),
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

  if (pthread_create (&pts[i], NULL, amt_vnew_threadv, 
		      static_cast<void *> (arg)) < 0)
    fatal ("mtdispatch::launch: pthread_create failed: %m\n");

  //close (closeit);
}

// XXX solve linker errors by making these functions noops...
#if HAVE_PTH
#include <pth.h>
int pth_init (void) { return -1; }
int pth_attr_set(pth_attr_t attr, int field, ...) { return -1; }
pth_t pth_spawn(pth_attr_t attr, void *(*entry)(void *), void *arg) 
{ return NULL; }
pth_attr_t pth_attr_new (void) { return NULL; }
#endif
