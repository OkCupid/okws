
#include "okdbg-int.h"
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
