

#include "okdbg.h"

static void
usage ()
{
  warnx << "usage: " << progname  << " [-abB]\n"
	<< "\t -b pub subsystem: dump bind table after each insert\n"
	<< "\t -a pub subsystem: dump bind table after all inserts\n"
	<< "\t -B pub subsystem: dump bind table before access\n"
	<< "\t -E pub subsystem: complain about errors\n"
	<< "\t -s pub subsystem: display shutdown status messages\n"
    ;
  exit (1);
}


int
main (int argc, char *argv[])
{
  int ch;
  int64_t res = 0;
setprogname (argv[0]);
  while ((ch = getopt (argc, argv, "abBEs")) != -1) {
    switch (ch) {
    case 'a':
      res = res | OKWS_DEBUG_PUB_BINDTAB_INSERTS;
      break;
    case 'b':
      res = res | OKWS_DEBUG_PUB_BINDTAB_INSERT;
      break;
    case 'B':
      res = res | OKWS_DEBUG_PUB_BINDTAB_ACCESS;
      break;
    case 'E':
      res = res | OKWS_DEBUG_PUB_ERRORS;
      break;
    case 's':
      res = res | OKWS_DEBUG_OKD_SHUTDOWN;
      break;
    default:
      usage ();
      break;
    }
  }

  printf ("OKWS_DEBUG_OPTIONS=0x%qx\n", res);
  return 0;
}

