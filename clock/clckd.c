
#include <sys/time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define CLCKD_INTERVAL 10000

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
main (int argc, char *argv[])
{
  char *mmap_rgn;
  int fd;
  struct timespec ts[2];
  struct timespec *targ;
  struct timeval wt;
  int d;

  //
  // make the file the right size by writing our time
  // there
  //
  clock_gettime (CLOCK_REALTIME, ts);
  ts[1]  = ts[0];

  if (argc != 2) 
    usage (argc, argv);
  fd = open (argv[1], O_RDWR|O_CREAT, 0644);
  if (fd < 0) {
    fprintf (stderr, "%s: cannot open file for reading\n", argv[1]);
    exit (1);
  }
  if (write (fd, (char *)ts, sizeof (ts)) != sizeof (ts)) {
    fprintf (stderr, "%s: short write\n", argv[1]);
    exit (1);
  }
  mmap_rgn = mmap (NULL, sizeof (struct timespec) * 2, PROT_WRITE,
		   MAP_SHARED, fd, 0); 
  if (mmap_rgn == MAP_FAILED) {
    fprintf (stderr, "mmap failed: %d\n", errno);
    exit (1);
  }

  targ = (struct timespec *) mmap_rgn;
  wt.tv_sec = 0;
  wt.tv_usec = CLCKD_INTERVAL;
  while (1) {
    clock_gettime (CLOCK_REALTIME, ts); 
    targ[0] = ts[0];
    targ[1] = ts[0];
    select (0, NULL, NULL, NULL, &wt);
    clock_gettime (CLOCK_REALTIME, ts+1); 
    d = timespec_diff (ts[1], ts[0]);
    if (d > 20000)
      fprintf (stderr, "%d: long sleep\n", d);
  }
}
