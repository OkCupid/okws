
#include "okxml.h"
#include "abuf_pipe.h"

static void usage (void)
{
  fatalx << "usage: " << progname << "\n";
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 1) 
    usage ();

  abuf_pipe_t src (ain);
  abuf_t (&src);
  

}
