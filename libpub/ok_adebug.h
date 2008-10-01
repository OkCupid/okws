// -*-c++-*-

#ifndef _LIBPUB_OK_ADEBUG_H_
#define _LIBPUB_OK_ADEBUG_H_

#include "amisc.h"
#include "tame.h"
#include "qhash.h"
#include <syslog.h>

namespace ok {

  class debug_obj_params_t {
  public:
    debug_obj_params_t (int fl, int fd, const char *lev)
      : _flags (fl), _fd (fd), _level (lev) {}
    int _flags, _fd;
    const char *_level;
  };

  class debug_obj_t : public strbuf {
  public:
    enum { xflag = 1, warnxflag = 2, timeflag = 8 };

    explicit debug_obj_t (int fl = -1, int fd = -1, const char *lev = NULL);
    explicit debug_obj_t (const debug_obj_params_t &p);
    ~debug_obj_t ();

    const debug_obj_t &operator() (const char *fmt, ...) const
      __attribute__ ((format (printf, 2, 3)));

  private:
    void init ();
    const int _flags;
    const int _fd;
    const char *_level;
  };

  class debug_fd_t {
  public:
    debug_fd_t (int fd) : _fd (fd), _flushing (false) {}
    void output (const suio *in, int flags);
  private:
    void flush (CLOSURE);

  public:
    int _fd;
    ihash_entry<debug_fd_t> _lnk;
  private:
    suio _uio;
    bool _flushing;
  };

  class debug_mgr_t {
  public:
    debug_mgr_t () {}
    ~debug_mgr_t () {}

    void output (const suio *in, int flags, int fd);

  private:
    debug_fd_t *get (int fd);
    ihash<int, debug_fd_t, &debug_fd_t::_fd, &debug_fd_t::_lnk> _tab;
  };

  int start_custom_logger (const str &pri, str tag = NULL);
  void setprogpid (int p = -1);

  class syslog_ctl_t {
  public:
    syslog_ctl_t ();
    bool enable_level (const str &l);
    bool start_loggers ();

    debug_obj_params_t params (int lev, bool x = false) const;


    enum { EMERG = 0,
	   ALERT = 1,
	   CRIT = 2,
	   ERR = 3,
	   WARNING = 4,
	   NOTICE = 5,
	   INFO = 6,
	   DEBUG = 7,
           NLEV = 8 };

    static const char *_levels[NLEV];

    int emerg_fd () { return _fds[EMERG]; }
    int alert_fd () { return _fds[ALERT]; }
    int crit_fd () { return _fds[CRIT]; }
    int err_fd () { return _fds[ERR]; }
    int warning_fd () { return _fds[WARNING]; }
    int notice_fd () { return _fds[NOTICE]; }
    int info_fd () { return _fds[INFO]; }
    int debug_fd () { return _fds[DEBUG]; }

    bool started () const { return _started; }

    str to_str () const;
    bool init_child (const str &s);

  private:
    bool _started;
    int _fds[NLEV];
    bool _set[NLEV];
    qhash<str, int> _tab;
  };

  void debug_startup (evv_t ev, CLOSURE);
  
  extern syslog_ctl_t syslog_ctl;
};

/* EMERGENCY level */
#define syslog_emerg \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.EMERG))
#define syslog_emerg_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.EMERG, true))

/* ALERT level */
#define syslog_alert \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.ALERT))
#define syslog_alert_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.ALERT, true))

/* CRIT level */
#define syslog_crit \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.CRIT))
#define syslog_crit_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.CRIT, true))

/* ERR level */
#define syslog_err \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.ERR))
#define syslog_err_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.ERR, true))

/* WARNING level */
#define syslog_warning \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.WARNING))
#define syslog_warning_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.WARNING, true))

/* NOTICE level */
#define syslog_notice \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.NOTICE))
#define syslog_notice_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.NOTICE, true))

/* INFO level */
#define syslog_info \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.INFO))
#define syslog_info_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.INFO, true))

/* DEBUG level */
#define syslog_debug \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.DEBUG))
#define syslog_debug_x \
  ok::debug_obj_t(ok::syslog_ctl.params (ok::syslog_ctl.DEBUG, true))


#endif /* _LIBPUB_OK_ADEBUG_H_ */
