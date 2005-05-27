

#include "okdbg.h"

static void
usage ()
{
  warnx << "usage: " << progname  << " [-abBEsS]\n"
	<< "\t -b pub subsystem: dump bind table after each insert\n"
	<< "\t -a pub subsystem: dump bind table after all inserts\n"
	<< "\t -B pub subsystem: dump bind table before access\n"
	<< "\t -E pub subsystem: complain about errors\n"
	<< "\t -S okd subsystem: display shutdown status messages\n"
	<< "\t -s okd subsystem: display startup status messages\n"
	<< "\t -c okd subsystem: noisy output about new connections\n"
	<< "\t -h hlp subsystem: noisy output about helper connections\n"
    ;
  exit (1);
}


int
main (int argc, char *argv[])
{
  int ch;
  int64_t res = 0;
  setprogname (argv[0]);
  while ((ch = getopt (argc, argv, "abBEsSch")) != -1) {
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
    case 'S':
      res = res | OKWS_DEBUG_OKD_SHUTDOWN;
      break;
    case 's':
      res = res | OKWS_DEBUG_OKD_STARTUP;
      break;
    case 'c':
      res = res | OKWS_DEBUG_OKD_NOISY_CONNECTIONS;
      break;
    case 'h':
      res = res | OKWS_DEBUG_HLP_STATUS;
      break;
    default:
      usage ();
      break;
    }
  }

  printf ("%s=0x%qx\n", OKWS_DEBUG_OPTIONS, res);
  return 0;
}

