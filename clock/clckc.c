
#include <sys/time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

struct timespec *mmap_clck_region;
int mmap_clck_init_flag = 0;
int mmap_clck_n_since_diff = 0;
struct timespec mmap_clck_last;
int mmap_clck_fd = -1;

#define MMAP_CLCK_TRIES  10
#define MMAP_CLCK_WAIT   1
#define MMAP_CLCK_STALE_THRESHHOLD  50000

void
usage (int argc, char *argv[])
{
  fprintf (stderr, "usage: %s clockfile\n", argv[0]);
  exit (1);
}

int 
timespec_diff (struct timespec a, struct timespec b)
{
  return  (a.tv_nsec - b.tv_nsec) / 1000 -
    (b.tv_sec - a.tv_sec) * 1000000;
}

int
mmap_clck_init (char *file)
{
  char *tmp;
  mmap_clck_fd = open (file, O_RDONLY);
  if (mmap_clck_fd < 0) {
    fprintf (stderr, "%s: file open failed with status %d\n", file, errno);
    return -1;
  }
  tmp = mmap (NULL, sizeof (struct timespec) * 2, PROT_READ, 
	      MAP_NOSYNC|MAP_SHARED, mmap_clck_fd, 0); 
  if (tmp == MAP_FAILED) {
    fprintf (stderr, "mmap failed: %d\n", errno);
    close (mmap_clck_fd);
    return -1;
  }
  mmap_clck_region = (struct timespec *)tmp;
  fprintf (stderr, "mmap clock initialzed!\n");
  mmap_clck_init_flag = 1;
  clock_gettime (CLOCK_REALTIME, &mmap_clck_last);
  return 0;
}

void
mmap_clck_fail ()
{
  mmap_clck_init_flag = 0;
  fprintf (stderr, "**mmap clock failed; reverting to stable\n");
  munmap (mmap_clck_region, sizeof (struct timespec) * 2 );
  close (mmap_clck_fd);
}

#define TIMESPEC_INC(ts)                  \
  if (++ (ts)->tv_nsec == 1000000000L)  { \
    (ts)->tv_sec ++;                      \
    (ts)->tv_nsec = 0L;                   \
  }                                    

#define TIMESPEC_LT(ts1, ts2)              \
  (((ts1).tv_sec < (ts2).tv_sec) ||       \
   ((ts1).tv_sec == (ts2).tv_sec && (ts1).tv_sec < (ts2).tv_sec))

#define TIMESPEC_EQ(ts1, ts2) \
  ((ts1).tv_sec == (ts2).tv_sec && (ts1).tv_nsec == (ts2).tv_nsec)

void
mmap_clck_gettime (struct timespec *out)
{
  int i;
  struct timeval wt;
  struct timespec tmp;
  int timeok = 0;

  wt.tv_sec = 0;
  wt.tv_usec = MMAP_CLCK_WAIT;

  if (!mmap_clck_init_flag) {
    clock_gettime (CLOCK_REALTIME, out);
    return;
  }

  for (i = 0; i < MMAP_CLCK_TRIES && !timeok; i++) {
    *out = mmap_clck_region[0];
    tmp = mmap_clck_region[1];

    //
    // likely case -- timestamp in shared memory is stale.
    // no sense in waiting since the time is going to be stale
    // anyways. 
    //
    if (TIMESPEC_LT (*out, mmap_clck_last)) {
      TIMESPEC_INC (&mmap_clck_last) ;
      *out = mmap_clck_last;
      if (++mmap_clck_n_since_diff > MMAP_CLCK_STALE_THRESHHOLD) {
	mmap_clck_fail ();
      }
      timeok = 1;

    } else if (TIMESPEC_EQ (*out, tmp)) {
      mmap_clck_last = *out;
      mmap_clck_n_since_diff = 0;
      timeok = 1;
    }

    // otherwise, let's wait for the times to equal out
    select (0, NULL, NULL, NULL, &wt);
  }

  if (i > 1)
    fprintf (stderr, "*** more than 1 try!\n");

  if (!timeok) 
    mmap_clck_fail ();

  if (!mmap_clck_init_flag) {
    clock_gettime (CLOCK_REALTIME, out);
    return;
  }
}

int
main (int argc, char *argv[])
{
  int i;
  int d;
  struct timespec ts, ts2;
  struct timeval tw;


  tw.tv_sec = 0;
  tw.tv_usec = 100000;

  if (argc != 2) 
    usage (argc, argv);
  if (mmap_clck_init (argv[1]) < 0) {
    fprintf (stderr, "%s: initialization failed\n", argv[1]);
    exit (1); 
  }
 
  for (i = 0; i < 1000; i++) {
    clock_gettime (CLOCK_REALTIME, &ts2);
    mmap_clck_gettime (&ts);
    d = timespec_diff (ts2, ts);
    printf ("%d.%d %d.%d %d.%d\n", ts2.tv_sec, ts2.tv_nsec, 
	    ts.tv_sec, ts.tv_nsec, d / 1000, d % 1000);
    select (0, NULL, NULL, NULL, &tw);
  }

  return (0);
}

