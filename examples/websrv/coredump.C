
#include "async.h"
#include "parseopt.h"

void
usage ()
{
  fatal << "usage: coredump [uid] [jaildir]\n";
}

int
main (int argc, char *argv[])
{
  int uid = -1;
  str jaildir;
  if (argc > 1 && !(convertint (argv[1], &uid)))
    usage ();
  else if (argc > 2)
    jaildir = argv[2];
  if (jaildir && chroot (jaildir) != 0)
    fatal << "cannot chroot to " << jaildir << "\n";
  else {
    if (chroot ("/run") != 0)
      fatal << "cannot chroot second time to /run\n";
    else if (uid > 0 && setuid (uid) != 0)
      fatal << "cannot setuid to " << uid << "\n";
  }

  panic ("dumping core\n");
}
