// -*-c++-*-

#ifndef _LIBPUB_OK_ADEBUG_H_
#define _LIBPUB_OK_ADEBUG_H_

#include "amisc.h"
#include "tame.h"
#include "qhash.h"

namespace ok {

  class debug_obj_t : public strbuf {
  public:
    enum { xflag = 1, timeflag = 8 };

    explicit debug_obj_t (int fl = -1, int fd = -1);
    ~debug_obj_t ();

    const debug_obj_t &operator() (const char *fmt, ...) const
      __attribute__ ((format (printf, 2, 3)));

  private:
    const int _flags;
    const int _fd;
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

};

#endif /* _LIBPUB_OK_ADEBUG_H_ */
