// -*-c++-*-

#include "ok_adebug.h"

//
// Code here copied from SFS, async/aerr.C
//

namespace ok {

  debug_mgr_t debug_mgr;

  debug_fd_t *
  debug_mgr_t::get (int i)
  {
    debug_fd_t *ret = _tab[i];
    if (!ret) {
      ret = New debug_fd_t (i);
      _tab.insert (ret);
    }
    return ret;
  }

  static bool
  ends_in_newline (suio *s, int l)
  {
    return s->iov ()[l].iov_len > 0 &&
      *((char *)s->iov ()[l].iov_base + s->iov()[l].iov_len - 1) == '\n';
  }

  void
  debug_fd_t::output (const suio *in, int flags)
  {
    // If the current uio ends in newline, then break it at the 
    // current place.
    if (_uio.resid () && ends_in_newline (&_uio, _uio.iovcnt () - 1))
      _uio.breakiov ();

    _uio.copyu (in);

    flush ();
  }

  tamed void
  debug_fd_t::flush ()
  {
    tvars {
      bool go (true);
      bool do_output (true);
      int cnt;
      int n;
    }

    if (_flushing) return;
    
    _flushing = true;
  
    while (go) {

      /* Try to write whole lines at a time. */
      for (cnt = min (_uio.iovcnt (), (size_t) UIO_MAXIOV);
	   cnt > 0 && !ends_in_newline (&_uio, cnt-1);
	   cnt--)
	;
      
      if (!cnt) {
	if (_uio.iovcnt () < UIO_MAXIOV) {
	  /* Wait for a carriage return */
	  do_output = false;
	  go = false;
	} else {
	  cnt = -1;
	}
      }
      
      if (do_output) {
	n = _uio.output (_fd, cnt);
	if (n < 0)
	  _uio.clear ();
	
	if (_uio.resid ()) {
	  twait { fdcb (_fd, selwrite, mkevent ()); }
	  fdcb (_fd, selwrite, NULL);
	} else {
	  go = false;
	}
      } 
    }
    _flushing = false;
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
