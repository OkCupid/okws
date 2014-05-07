#include "okdbg-int.h"
#include "okdbg.h"
#include "parseopt.h"
#include "okformat.h"

void
set_debug_flags ()
{
  str df = getenv (OKWS_DEBUG_OPTIONS);
  if (df && !convertint (df, &okws_debug_flags)) {
    warn << "invalid debug flags given: " << df << "\n";
  }
  if (okws_debug_flags)
    warn ("OKWS debug flags set: 0x%" PRIx64 "\n", okws_debug_flags);
}

void 
okdbg_warn (okdbg_lev_t l, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  strbuf b;
  b.vfmt (fmt, ap);
  va_end (ap);
  okdbg_warn (l, b);
}

void
okdbg_warn (okdbg_lev_t l, const str &s)
{
  size_t len = s.len ();
  char *cp = New char[len + 1];

  if (!cp) {
    warn << "MALLOC FAILED! Warn anyway: " << s << "\n";
    return;
  }

  memcpy (cp, s.cstr (), len + 1);

  char *bp, *np;

  for (bp = cp; bp && *bp; bp = np) {
    if ((np = strchr (bp, '\n'))) {
      *np++ = '\0';
    }
    switch (l) {
    case CHATTER:
      warn << "++ ";
      break;
    case ERROR:
      warn << "** ";
      break;
    case FATAL_ERROR:
      warn << "XX ";
      break;
    default:
      warn << "";
      break;
    }
    warnx << bp << "\n";
  }
  delete [] cp;
}

//-----------------------------------------------------------------------

bool
okdbg_t::flag2bits (char c, int64_t *out) const
{
  bool ret = true;
  int64_t v;
  
  switch (c) {
  case 'a': v = OKWS_DEBUG_PUB_BINDTAB_INSERTS; break;
  case 'b': v = OKWS_DEBUG_PUB_BINDTAB_INSERT; break;
  case 'c': v = OKWS_DEBUG_OKD_NOISY_CONNECTIONS; break;
  case 'd': v = OKWS_DEBUG_SVC_DATABASES; break;
  case 'k': v = OKWS_DEBUG_OKD_KEEPALIVE; break;
  case 'v': v = OKWS_DEBUG_SVC_STARTUP; break;
  case 'f': v = OKWS_DEBUG_OKLD_FD_PASSING; break;
  case 'h': v = OKWS_DEBUG_HLP_STATUS; break;
  case 'i': v = OKWS_DEBUG_SSL_INDATA; break;
  case 'm': v = OKWS_DEBUG_SSL_MEM; break;
  case 'o': v = OKWS_DEBUG_SSL_OUTDATA; break;
  case 'p': v = OKWS_DEBUG_SSL_PROXY; break;
  case 's': v = OKWS_DEBUG_OKD_STARTUP; break;
  case 'u': v = OKWS_DEBUG_STALL_SIGCONT; break;
  case 'A': v = OKWS_DEBUG_SVC_ARGS; break;
  case 'B': v = OKWS_DEBUG_PUB_BINDTAB_ACCESS; break;
  case 'C': v = OKWS_DEBUG_PUB3_CACHE; break;
  case 'H': v = OKWS_DEBUG_PUB3_CHUNKS; break;
  case 'E': v = OKWS_DEBUG_PUB_ERRORS; break;
  case 'P': v = OKWS_DEBUG_PUB_PARSE; break;
  case 'J': v = OKWS_DEBUG_OKD_JAIL; break;
  case 'S': v = OKWS_DEBUG_OKD_SHUTDOWN; break;
  case 'M': v = OKWS_DEBUG_SVC_MPFD; break;
  case 'N': v = OKWS_DEBUG_OKD_CHILDREN; break;
  default: ret =  false; break;
  }
  
  if (ret) *out |= v;
  return ret;
}

//--------------------------------------------------------------------

const char * 
okdbg_t::allflags () const
{
  return "abcdfhikmopsuvABCEHJMPSN";
}

//--------------------------------------------------------------------

const char *
okdbg_t::documentation () const
{
  const char *ret = 
    "\t -a pub subsystem: dump bind table after all inserts\n"
    "\t -b pub subsystem: dump bind table after each insert\n"
    "\t -B pub subsystem: dump bind table before access\n"
    "\t -E pub subsystem: complain about errors\n"
    "\t -C pb2 subsystem: display messages about pub2's cache\n"
    "\t -H pb2 subsystem: display messages about pub2 chunking\n"
    "\t -P pub sussystem: bison/parse debug output\n"
    "\n"
    "\t -c okd subsystem: noisy output about new connections\n"
    "\t -k okd subsystem: keepalive internals\n"
    "\t -s okd subsystem: display startup status messages\n"
    "\t -J okd subsystem: debug jail2real() calls\n"
    "\t -S okd subsystem: display shutdown status messages\n"
    "\n"
    "\t -A svc subsystem: dump argument list on startup\n"
    "\t -d svc subsystem: debug database connections\n"
    "\t -v svc subsystem: startup debug messages\n"
    "\t -M svc subsystem: debug multipart data internals\n"
    "\n"
    "\t -m ssl subsystem: debug memory allocations\n"
    "\t -p ssl subsystem: debug proxy operations\n"
    "\t -i ssl subsystem: debug SSL input data\n"
    "\t -o ssl subsystem: debug SSL output data\n"
    "\n"
    "\t -f kld subsystem: display FD passing information\n"
    "\t -h hlp subsystem: noisy output about helper connections\n"
    "\t -u general flag : stall until CONT signal\n"
    ;
  return ret;
}

//-----------------------------------------------------------------------

void
okdbg_t::usage ()
{
  warnx << "usage: " << progname  << " [-" << allflags () << "]\n";
  warnx << documentation ();
  exit (1);
}

//-----------------------------------------------------------------------

int 
okdbg_t::main (int argc, char *argv[])
{
  
  int ch;
  int64_t res = 0;
  setprogname (argv[0]);
  while ((ch = getopt (argc, argv, allflags ())) != -1) {
    if (!flag2bits (ch, &res))
      usage ();
  }
  printf ("%s=0x%" PRIx64 "\n", OKWS_DEBUG_OPTIONS, res);
  return 0;
}

//-----------------------------------------------------------------------


