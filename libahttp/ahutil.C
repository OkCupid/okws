
#include <time.h>
#include <stdio.h>
#include "ahutil.h"

str
getdate ()
{
  const time_t t1 = time (NULL);
  struct tm *t2 = gmtime (&t1);
  static char buf[100];
  size_t n = strftime (buf, 100, "%a, %d %b %Y %T %Z", t2);
  return str (buf, n);
}

bool
mystrlcmp (const str &s, const char *b)
{
  const char *a = s.cstr ();
  const char *e = a + s.len ();
  while (a < e) 
    if (tolower (*a++) != tolower (*b++))
      return false;
  return true;
}

str
tolower (const str &s)
{
  mstr m (s.len ());
  char *cp;
  const char *cp2;
  for (cp = m.cstr (), cp2 = s.cstr (); *cp2; cp++, cp2++)
    *cp = tolower (*cp2);
  return m;
}

static void
stallcb1 (int sig, cbv c)
{
  warn << "Caught signal (" << sig << "), proceeding.\n";
  sigcb (sig, NULL);
  (*c) ();
}

void
stall (int sig, cbv c)
{
  warn << "Process PID=" << getpid () << " stalled, waiting "
       << "for signal " << sig << "\n";
  sigcb (sig, wrap (stallcb1, sig, c));
}

static void
stallcb2 (str fn, cbv c)
{
  if (access (fn.cstr (), R_OK) == 0) {
    warn << "Found file (" << fn << "), proceeding.\n";
    unlink (fn.cstr ());
    (*c) ();
  } else {
    delaycb (1, wrap (stallcb2, fn, c));
  }
}

void
stall (const str &fn, cbv c)
{
  warn << "Stalling (pid: " << getpid () << 
    "), checking for file: " << fn << "\n";
  stallcb2 (fn, c);
}

