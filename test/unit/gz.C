
#include "async.h"
#include "serial.h"
#include "zstr.h"

static void
usage (const char *cmd)
{
  warnx << "usage: " << cmd << " <file>\n";
  exit (-1);
}

static void
run_test (str dat)
{
  warn << "Input Size: " <<  dat.len () << "\n";
  str out = zcompress (dat, 9);
  warn << "Compressed Size: " << out.len () << "\n";
  str dat2 = zdecompress (out);

  if (!dat2) {
    warn << "Unhappy! Decompress failed!\n";
  } else {
    warn << "Decompress Size: " << dat2.len () << "\n";

    if (dat2 != dat) {
      warn << "mismatch!! D'oh!\n";
    }
  }
}


int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 2) {
    usage (argv[0]);
  }
  zinit ();
 
  str dat = file2str (argv[1]);
  if (!dat) {
    warn << "Could not read file: " << argv[1] << "\n";
    return -1;
  }

  run_test (dat);
  return 0;
}
