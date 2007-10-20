
// -*-c++-*-
#include "ok_adebug.h"

namespace ok {

  debug_mgr_t debug_mgr;

  suio *
  debug_mgr_t::get (int i)
  {
    suio **p = _tab[i];
    suio *ret;
    if (p) {
      ret = *p;
    } else {
      ret = New suio ();
      _tab.insert (i, ret);
    }
    return ret;
  }

  static bool
  ends_in_newline (suio *s, int l)
  {
    return 
      s->iov ()[l].iov_len > 0 &&
      *((char *)s->iov ()[l].iov_base + s->iov()[l].iov_len - 1) == '\n';
  }

  tamed void
  debug_mgr_t::output (const suio *in, int flags, int fd)
  {
    tvars {
      suio *guio;
      int cnt;
      int n;
      bool go (true);
    }

    guio = get (fd);
    assert (guio);

    // If the current uio ends in newline, then break it at the 
    // current place.
    if (guio->resid () && ends_in_newline (guio, guio->iovcnt () - 1))
      guio->breakiov ();

    guio->copyu (in);

    while (go) {

      /* Try to write whole lines at a time. */
      for (cnt = min (guio->iovcnt (), (size_t) UIO_MAXIOV);
	   cnt > 0 && !ends_in_newline (guio, cnt-1);
	   cnt--)
	;
      
      if (!cnt) {
	if (guio->iovcnt () < UIO_MAXIOV) {
	  /* Wait for a carriage return */
	  return;
	} else {
	  cnt = -1;
	}
      }
      
      n = guio->output (fd, cnt);
      if (n < 0)
	guio->clear ();
      
      if (guio->resid ()) {
	twait { fdcb (fd, selwrite, mkevent ()); }
	fdcb (fd, selwrite, NULL);
      } else {
	go = false;
      }
    }

  }

  static const char *
  timestring ()
  {
    timespec ts;
    clock_gettime (CLOCK_REALTIME, &ts);
    static str buf;
    buf = strbuf ("%d.%06d", int (ts.tv_sec), int (ts.tv_nsec/1000));
    return buf;
  }

  debug_obj_t::debug_obj_t (int fl, int fd)
    : _flags (fl), _fd (fd)
  {
    int progpid = getpid ();
    if (_flags & timeflag)
      cat (timestring ()).cat (" ");
    if (!(_flags & xflag) && progname)
      if (progpid)
	cat (progname).cat ("[").cat (progpid).cat ("]: ");
      else
	cat (progname).cat (": ");
  }
  
  const debug_obj_t &
  debug_obj_t::operator() (const char *fmt, ...) const
  {
    va_list ap;
    va_start (ap, fmt);
    vfmt (fmt, ap);
    va_end (ap);
    return *this;
  }
  
  debug_obj_t::~debug_obj_t ()
  {
    debug_mgr.output (uio, _flags, _fd);
  }

};
