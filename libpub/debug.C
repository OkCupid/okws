
#include "okdbg-int.h"
#include "parseopt.h"

void
set_debug_flags ()
{
  str df = getenv (OKWS_DEBUG_OPTIONS);
  if (df && !convertint (df, &okws_debug_flags)) {
    warn << "invalid debug flags given: " << df << "\n";
  }
  if (okws_debug_flags)
    warn ("OKWS debug flags set: 0x%qx\n", okws_debug_flags);
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
  vec<str> lines;
  static rxx newline ("\\n");
  split (&lines, newline, s);
  for (size_t i = 0; i < lines.size (); i++) {
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
    warnx << lines[i] << "\n";
  }
}
