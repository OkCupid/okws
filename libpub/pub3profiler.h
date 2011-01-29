// -*-c++-*-
/* $Id$ */

#pragma once

#include "pub3eval.h"
#include "sfs_profiler.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class profiler_buf_t {
  public:
    profiler_buf_t (size_t sz = 0);
    ~profiler_buf_t ();
    void add_s (str s);
    void add_ch (char c);
    void add_i (int i);
    void add_cc (const char *c);
    void flush ();
    void report ();
    void recharge ();
    void reset () ;
  private:
    size_t room () const { return _end - _bp; }
    bool check_room (size_t n);
    size_t _sz;
    char *_buf;
    char *_bp;
    char *_end;
    bool _overflow;
    bool _trunced;
  };

  //-----------------------------------------------------------------------

  class profiler_t : public sfs_profiler::core_t {
  public:
    profiler_t ();
    ~profiler_t ();
    static profiler_t *singleton ();
    static profiler_t *profiler () { return singleton (); }
    void register_stack (loc_stack_t *s);
    void unregister_stack (loc_stack_t *s);
    bool enable (time_t msec = 0);
    void disable ();
    void profile_hook (const void *v);
    void recharge ();
    void report ();
    void reset ();
  private:
    list<loc_stack_t, &loc_stack_t::_lnk> _stack_list;
    u_int64_t _seqid;
    profiler_buf_t _buf;
  };

  //-----------------------------------------------------------------------

};
