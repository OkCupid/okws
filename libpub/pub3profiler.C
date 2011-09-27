// -*-c++-*-

#include "pub3eval.h"
#include "pub3out.h"
#include "pub3hilev.h"
#include "pub3profiler.h"
#include "sfs_profiler.h"

namespace pub3 {

  //======================================= profiler_buf_t ==============

  profiler_buf_t::profiler_buf_t (size_t sz)
    : _sz (sz ? sz : ok_pub3_profiler_buf_minsize),
      _buf (New char[_sz]),
      _bp (_buf),
      _end (_buf + _sz),
      _overflow (false),
      _trunced (false)
  {
    memset (_buf, 0, _sz);
  }

  //---------------------------------------------------------------------

  void
  profiler_buf_t::add_s (str s)
  {
    size_t l;
    if (!s || !(l = s.len ())) { /* noop */ }
    else if (check_room (l)) {
      memcpy (_bp, s.cstr(), l);
      _bp += l;
    }
    assert (_bp <= _end);
  }

  //---------------------------------------------------------------------

  void
  profiler_buf_t::add_cc (const char *c)
  {
    size_t l = strlen (c);
    if (check_room (l)) {
      memcpy (_bp, c, l);
      _bp += l;
    }
    assert (_bp <= _end);
  }

  //---------------------------------------------------------------------

  bool
  profiler_buf_t::check_room (size_t s)
  {
    bool ret = true;
    if (_overflow) { ret = false; }
    else if (s > room ()) { 
      _overflow = true; 
      ret = false; 
      _trunced = true;
    }
    return  ret;
  }

  //---------------------------------------------------------------------

  void
  profiler_buf_t::add_i (int i)
  {
#define BUFSZ 0x100
    static char tmp[BUFSZ];
    size_t nc = snprintf (tmp, BUFSZ, "%d", i);
    if (check_room (nc)) {
      memcpy (_bp, tmp, nc);
      _bp += nc;
    }
    assert (_bp <= _end);
#undef BUFSZ
  }

  //---------------------------------------------------------------------

  void
  profiler_buf_t::add_ch (char ch)
  {
    if (check_room (1)) {
      *(_bp++) = ch;
    }
    assert (_bp <= _end);
  }

  //---------------------------------------------------------------------

  profiler_buf_t::~profiler_buf_t () 
  {
    delete [] _buf;
  }

  //---------------------------------------------------------------------

  void
  profiler_buf_t::reset ()
  {
    _bp = _buf;
    _overflow = false;
    _trunced = false;
  }
  
  //---------------------------------------------------------------------

  void 
  profiler_buf_t::recharge ()
  {
    if (_overflow && _sz * 2 <  ok_pub3_profiler_buf_maxsize) {
      size_t ns = _sz * 2;

      // Make sure we didn't fuck up in the past
      assert (_bp <= _end);

      char *newb = New char[ns];
      size_t datsz = _bp - _buf;
      memcpy (newb, _buf, datsz);
      memset (newb + datsz, 0, ns - datsz);
      delete [] _buf;
      _sz = ns;
      _buf = newb;
      _end = _buf + _sz;
      _bp = _buf + datsz;

      // make sure that the new buffer pointer is in bounds!
      assert (_bp <= _end);

      _overflow = false;
    }
  }

  //---------------------------------------------------------------------

  void profiler_buf_t::flush () { add_ch ('\0'); }

  //---------------------------------------------------------------------

  void
  profiler_buf_t::report ()
  {
    const char *prefix = "(SPP) ";
    warn << prefix << "++++++ start report +++++++++++++++++ \n";
    char *last = _buf;
    for (char *cp = _buf; cp < _bp; cp++) {
      if (*cp == '\0' && cp > last) {
	warn << prefix << last << "\n";
	last = cp + 1;
      }
    }

    if (_trunced) {
      warn << prefix << "report was truncated....\n";
    }
    warn << prefix << "----------- end report -----------------\n";
    _bp = _buf;
    _overflow = false;
    _trunced = false;
  }

  //======================================= profiler_t ==================


  profiler_t::profiler_t () : _seqid (0) {}
  profiler_t::~profiler_t () {}

  //-----------------------------------------------------------------------

  static profiler_t *g_profiler;

  profiler_t *
  profiler_t::singleton ()
  {
    if (!g_profiler) { g_profiler = New profiler_t(); }
    return g_profiler; 
  }

  //-----------------------------------------------------------------------

  void profiler_t::register_stack (loc_stack_t *s)
  { _stack_list.insert_head (s); }

  //-----------------------------------------------------------------------

  void profiler_t::unregister_stack (loc_stack_t *s)
  { _stack_list.remove (s); }

  //-----------------------------------------------------------------------

  void
  profiler_t::profile_hook (const void *context) 
  {
    for (loc_stack_t *p = _stack_list.first; p; p = _stack_list.next (p)) {
      p->profile_report (&_buf, _seqid);
    }
    _seqid++;
  }

  //-----------------------------------------------------------------------

  bool
  profiler_t::enable (time_t ms)
  {
    bool ret = true;
    if (!ms) { ms = ok_pub3_profiler_interval_msec; }
    sfs_profiler::set_core (this);
    sfs_profiler::set_interval (ms * 1000);
    if (!sfs_profiler::enable ()) {
      warn << "Failed to enable profiler; check that SFS profiler is enabled\n";
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void profiler_t::disable () 
  { 
    sfs_profiler::disable (); 
    sfs_profiler::set_core (NULL);
  }

  //-----------------------------------------------------------------------

  void profiler_t::recharge () { _buf.recharge (); }
  void profiler_t::report () { _buf.report (); }
  void profiler_t::reset () { _buf.reset (); }

  //-----------------------------------------------------------------------

  //=====================================================================

};
