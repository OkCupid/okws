
#include "okdbg.h"
#include "okformat.h"


static void
usage ()
{
  warnx << "usage: " << progname  << " [-abcdfhmpsuvABEJS ]\n"
	<< "\t -a pub subsystem: dump bind table after all inserts\n"
	<< "\t -b pub subsystem: dump bind table after each insert\n"
	<< "\t -c okd subsystem: noisy output about new connections\n"
	<< "\t -f kld subsystem: display FD passing information\n"
	<< "\t -h hlp subsystem: noisy output about helper connections\n"
	<< "\t -s okd subsystem: display startup status messages\n"
        << "\t -u general flag : stall until CONT signal\n"
	<< "\t -A svc subsystem: dump argument list on startup\n"
	<< "\t -d svc subsystem: debug database connections\n"
	<< "\t -v svc subsystem: startup debug messages\n"
	<< "\t -C pb2 subsystem: display messages about pub2's cache\n"
	<< "\t -B pub subsystem: dump bind table before access\n"
	<< "\t -E pub subsystem: complain about errors\n"
	<< "\t -J okd subsystem: debug jail2real() calls\n"
	<< "\t -S okd subsystem: display shutdown status messages\n"
	<< "\t -m ssl subsystem: debug memory allocations\n"
	<< "\t -p ssl subsystem: debug proxy operations\n"
    ;
  exit (1);
}


int
main (int argc, char *argv[])
{
  int ch;
  int64_t res = 0;
  setprogname (argv[0]);
  while ((ch = getopt (argc, argv, "abcdfhmpsuvABCEJS")) != -1) {
    switch (ch) {
    case 'a':
      res = res | OKWS_DEBUG_PUB_BINDTAB_INSERTS;
      break;
    case 'b':
      res = res | OKWS_DEBUG_PUB_BINDTAB_INSERT;
      break;
    case 'c':
      res = res | OKWS_DEBUG_OKD_NOISY_CONNECTIONS;
      break;
    case 'd':
      res = res | OKWS_DEBUG_SVC_DATABASES;
      break;
    case 'v':
      res = res | OKWS_DEBUG_SVC_STARTUP;
      break;
    case 'f':
      res = res | OKWS_DEBUG_OKLD_FD_PASSING;
      break;
    case 'h':
      res = res | OKWS_DEBUG_HLP_STATUS;
      break;
    case 'm':
      res = res | OKWS_DEBUG_SSL_MEM;
      break;
    case 'p':
      res = res | OKWS_DEBUG_SSL_PROXY;
      break;
    case 's':
      res = res | OKWS_DEBUG_OKD_STARTUP;
      break;
    case 'u':
      res = res | OKWS_DEBUG_STALL_SIGCONT;
      break;
    case 'A':
      res = res | OKWS_DEBUG_SVC_ARGS;
      break;
    case 'B':
      res = res | OKWS_DEBUG_PUB_BINDTAB_ACCESS;
      break;
    case 'C':
      res = res | OKWS_DEBUG_PUB2_CACHE;
      break;
    case 'E':
      res = res | OKWS_DEBUG_PUB_ERRORS;
      break;
    case 'J':
      res = res | OKWS_DEBUG_OKD_JAIL;
      break;
    case 'S':
      res = res | OKWS_DEBUG_OKD_SHUTDOWN;
      break;
    default:
      usage ();
      break;
    }
  }

  printf ("%s=0x%" PRIx64 "\n", OKWS_DEBUG_OPTIONS, res);
  return 0;
}

