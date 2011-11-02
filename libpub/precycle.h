// -*-c++-*-
/* $Id: precycle.h 2784 2007-04-20 16:32:00Z max $ */

#ifndef _LIBPUB_PRECYCLE_H_
#define _LIBPUB_PRECYCLE_H_

#include "vec.h"

template<class T>
class recycler_t {
public:
  recycler_t (size_t s, str name) : 
      _lim (s),
      _alive (true),
      _show_stats(false),
      _stats_interval(10000),
      _name(name),
      _verbose(false) { _v.reserve (s/2); }

  ~recycler_t () { _alive = false; }
  
  ptr<T> alloc ()
  {
    ptr<T> ret;
    bool b = can_alloc();
    if (_show_stats) { alloc_hook(b); }
    if (can_alloc()) {
      ret = _v.pop_back ();
      ret->init ();
    } else {
      ret = New refcounted<T> ();
    }
    return ret;
  }

  template<class A1>
  ptr<T> alloc (const A1 &a1) 
  {
    ptr<T> ret;
    bool b = can_alloc();
    if (_show_stats) { alloc_hook(b); }
    if (can_alloc()) {
      ret = _v.pop_back ();
      ret->init (a1);
    } else {
      ret = New refcounted<T> (a1);
    }
    return ret;
  }

  template<class A1, class A2>
  ptr<T> alloc (const A1 &a1, const A2 &a2) 
  {
    ptr<T> ret;
    bool b = can_alloc();
    if (_show_stats) { alloc_hook(b); }
    if (can_alloc()) {
      ret = _v.pop_back ();
      ret->init (a1, a2);
    } else {
      ret = New refcounted<T> (a1, a2);
    }
    return ret;
  }

  template<class A1, class A2, class A3>
  ptr<T> alloc (const A1 &a1, const A2 &a2, const A3 &a3) 
  {
    ptr<T> ret;
    bool b = can_alloc();
    if (_show_stats) { alloc_hook(b); }
    if (b) {
      ret = _v.pop_back ();
      ret->init (a1, a2, a3);
    } else {
      ret = New refcounted<T> (a1, a2, a3);
    }
  }

  void recycle (T *o)
  {    
    bool b = can_recycle();
    if (_show_stats) { recycle_hook(b); }
    if (b) {
      _v.push_back (mkref (o));
    } else {
      delete o;
    }
  }


  size_t size() { return _v.size(); }
  bool can_alloc() { return _v.size(); }
  bool can_recycle() { return _alive && (_v.size() < _lim); }

  void set_limit(size_t limit) { _lim = limit; }
  void toggle_stats(bool enabled) { _show_stats = enabled; }
  void set_stats_interval(uint64_t interval) { _stats_interval = interval; }
  void set_verbose(bool verbose) { _verbose = verbose; }

private:
  void reset_stats() {
    _num_events = 0;
    _num_new = 0;
    _num_delete = 0;
    _num_avg_samples = 0;
    _list_size_sum = 0;
    _max_size_seen = 0;
  }

  void handle_event() {
    ++_num_events;

    // Sample list size
    if (_num_events % (_stats_interval / 100) == 0) {
      ++_num_avg_samples;
      _list_size_sum += size();

      if (size() > _max_size_seen) { 
        _max_size_seen = size();
      }
    }

    // Dump stats and then reset counters;
    if (_num_events % _stats_interval == 0) {
      okdbg_warn(CHATTER, 
             strbuf(_name) << ": Recycler stats (for " 
                           << _stats_interval << " events)\n");
      okdbg_warn(CHATTER, 
             strbuf(_name) << ":   calls to new: " << _num_new << "\n");
      okdbg_warn(CHATTER, 
             strbuf(_name) << ":   calls to delete: " << _num_delete << "\n");
      double avg = _list_size_sum / _num_avg_samples;
      okdbg_warn(CHATTER, 
             strbuf(_name) << ":   avg list size: " << avg << "\n");
      okdbg_warn(CHATTER, 
             strbuf(_name) << ":   max list size: " << _max_size_seen << "\n");

      // Resets _num_events to 0
      reset_stats();
    }
  }

  void alloc_hook(bool can_alloc) {
    handle_event();
    if (can_alloc) {
      if (_verbose) { 
        okdbg_warn(CHATTER, 
                   strbuf(_name) << ": grabbing obj from trash list"
                                 << " at pos " << this->size() - 1 << "\n");
      }
    } else {
      if (_verbose) {
        okdbg_warn(CHATTER, 
                   strbuf(_name) << ": allocating obj (with New)\n");
      }
      _num_new++;
    }
  }

  void recycle_hook(bool can_recycle) {
    handle_event();
    if (can_recycle) {
      if (_verbose) {
        okdbg_warn(CHATTER, 
                   strbuf(_name) << ": storing obj on trash list"
                                 << " at pos " << this->size() << "\n");
      }
    } else {
      if (_verbose) {
        okdbg_warn(CHATTER, 
                   strbuf(_name) << ": deleting obj\n");
      }
      _num_delete++;
    }
  }

  vec<ptr<T> > _v;
  size_t _lim;
  bool _alive;

  bool _show_stats;
  uint64_t _stats_interval;

  str _name;
  bool _verbose;
  uint64_t _num_events;
  uint64_t _num_new;
  uint64_t _num_delete;
  size_t _max_size_seen;
  double _list_size_sum;
  uint64_t _num_avg_samples;
};

//-----------------------------------------------------------------------------

#endif /* _LIBPUB_PRECYCLE_H_ */
