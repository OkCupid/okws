// -*-c++-*-

#include "ok_adebug.h"
#include "okconst.h"
#include "pubutil.h"
#include "rxx.h"
#include "parseopt.h"
#include "tame_io.h"

//
// Code here copied from SFS, async/aerr.C
//

namespace ok {

  debug_mgr_t debug_mgr;
  syslog_ctl_t syslog_ctl;
  int progpid;

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
    return buf.cstr();
  }

  debug_obj_t::debug_obj_t (int fl, int fd, const char *l)
    : _flags (fl), _fd (fd), _level (l)
  {
    init ();
  }

  debug_obj_t::debug_obj_t (const debug_obj_params_t &p)
    : _flags (p._flags), _fd (p._fd), _level (p._level)
  {
    init ();
  }

  void
  debug_obj_t::init ()
  {
    bool need_colon = false;
    bool need_space = false;
    if (_flags & timeflag) {
      cat (timestring ());
      need_space = true;
    }
    if (!(_flags & xflag) && progname) {
      need_colon = true;
      need_space = true;
      if (progpid)
	cat (progname).cat ("[").cat (progpid).cat ("]");
      else
	cat (progname);
    }
    if (_level && (_flags & warnxflag) && !(_flags & xflag)) {
      if (need_space) {
	cat (" ");
      }
      cat ("[").cat (_level).cat ("]");
      need_colon = true;
      need_space = false;
    }
    if (need_colon) {
      cat(": ");
    } else if (need_space) {
      cat (" ");
    }
  }

  void
  debug_mgr_t::output (const suio *u, int flags, int fd)
  {
    get (fd)->output (u, flags);
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
    if ((_flags & warnxflag)) {
      warnx << *this;
    } else {
      debug_mgr.output (uio, _flags, _fd);
    }
  }

  void 
  setprogpid (int p)
  {
    if (p < 0) 
      p = getpid ();
    progpid = p;
  }

  int 
  start_custom_logger (const str &priority, str tag)
  {
    int ret = -1;
#ifdef PATH_LOGGER
    if (!tag) tag = "";
    const char *av[] = { PATH_LOGGER, "-p", 
			 priority.cstr (), 
			 "-t", 
			 tag.cstr (), NULL };
    int fds[2];
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds) < 0) 
      fatal ("socketpair: %m\n");
    //close_on_exec (fds[0]);
    if (fds[1] != 0) 
      close_on_exec (fds[1]);
    
    if (spawn (PATH_LOGGER, av, fds[1], 0, 0) >= 0) {
      close (fds[1]);
      ret = fds[0];
    } else {
      ret = -1;
    }
#endif /* PATH_LOGGER */
    return ret;
  }

  syslog_ctl_t::syslog_ctl_t ()
    : _started (false)
  {
    for (int i = 0; i < NLEV; i++) {
      _fds[i] = errfd;
      _set[i] = false;
      _tab.insert (my_tolower (_levels[i]), i);
    }
  }

  const char *syslog_ctl_t::_levels[] = 
    { "emerg", "alert", "crit", "err", "warning", "notice", "info", "debug" } ;

  bool
  syslog_ctl_t::enable_level (const str &l)
  {
    int *lev = _tab[my_tolower(l)];
    if (lev) {
      _set[*lev] = true;
    }
    return (lev != NULL);
  }

  debug_obj_params_t 
  syslog_ctl_t::params (int lev, bool x) const
  {
    int mode = 0;
    if (x)
      mode |= debug_obj_t::xflag;

    if (!_started || !_set[lev])
      mode |= debug_obj_t::warnxflag;

    return debug_obj_params_t (mode, _fds[lev], _levels[lev]);
  }

  bool
  syslog_ctl_t::start_loggers ()
  {
    for (int i = 0; i < NLEV; i++) {
      if (_set[i]) {
	strbuf b;
	b << ok_syslog_domain << "." << _levels[i];
	if ((_fds[i] = start_custom_logger (b)) < 0) {
	  warn << "Could not start log level: " << b << "\n";
	  return false;
	}
      }
    }
    _started = true;
    return true;
  }

  bool 
  syslog_ctl_t::init_child (const str &s)
  {
    vec<str> v;
    static rxx sep (";");
    static rxx pair ("([0-9]+)=([0-9]+)");
    int lev (0), fd (0);
    bool ret = true;
    if (!s || s.len() == 0) {
      ret = true;
    } else if (split (&v, sep, s) <= 0) {
      ret = false; 
    } else {
      for (size_t i = 0; i < v.size (); i++) {
	if (!pair.match (v[i]) ||
	    !convertint (pair[1], &lev) ||
	    !convertint (pair[2], &fd)) {
	  ret = false;
	} else {
	  _set[lev] = true;
	  _fds[lev] = fd;
	}
      }
    }
    if (ret)
      _started = true;
    return ret;
  }

  str
  syslog_ctl_t::to_str () const
  {
    strbuf b;
    bool first = true;
    for (int i = 0; i < NLEV; i++) {
      if (_set[i]) {
	if (!first) b << ";";
	else first = false;
	b << i << "=" << _fds[i];
      }
    }
    return b;
  }

  tamed void
  debug_startup (evv_t ev)
  {
    warn << "DEBUG: waiting for SIGCONT (pid=" << getpid () << ")\n";
    twait {
      tame::sigcb1 (SIGCONT, mkevent ());
    }
    ev->trigger ();
  }

};
