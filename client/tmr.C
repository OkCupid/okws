
#include "async.h"
#include <sys/time.h>

struct timespec myts;
struct timeval mytv;

void
set_myts ()
{
  clock_gettime (CLOCK_REALTIME, &myts);
}

void
timer_event ()
{
  set_myts ();
  warn << myts.tv_sec << "." << myts.tv_nsec << "\n";
}

void
set_timer ()
{
  struct itimerval val;
  val.it_value.tv_sec = 0;
  val.it_value.tv_usec = 500000;
  val.it_interval.tv_sec = 0;
  val.it_interval.tv_usec = 500000;

  setitimer (ITIMER_REAL, &val, 0);
}


int
main (int argc, char *argv[])
{
  sigcb (SIGALRM, wrap (timer_event));
  set_timer ();
  amain ();
}

