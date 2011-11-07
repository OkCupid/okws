// -*-c++-*-
/* $Id: precycle.h 2784 2007-04-20 16:32:00Z max $ */

#ifndef _LIBPUB_PRECYCLE_H_
#define _LIBPUB_PRECYCLE_H_

#include "vec.h"

// Common stat reporting utilities for recyclers
class recycler_stats_t {
public:
  recycler_stats_t(str name) :
    _show_stats(false),
    _stats_interval(10000),
    _name(name),
    _verbose(false) { reset_stats(); }

  virtual ~recycler_stats_t () {}

  bool show_stats() const { return _show_stats; }
  void toggle_stats(bool enabled) { _show_stats = enabled; }
  void set_stats_interval(uint64_t interval) { _stats_interval = interval; }
  void set_verbose(bool verbose) { _verbose = verbose; }

protected:
  void reset_stats();
  void handle_event(size_t size);
  void alloc_hook(bool can_alloc, size_t size);
  void recycle_hook(bool can_recycle, size_t size);

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

// Recycler for refcounted type T
template<class T>
class recycler_t : public recycler_stats_t {
public:
  recycler_t (size_t s, str name) : 
      recycler_stats_t(name),
      _lim (s),
      _alive (true)
  { _v.reserve (s/2); }

  ~recycler_t () { _alive = false; }
  
  ptr<T> alloc ()
  {
    ptr<T> ret;
    bool b = can_alloc();
    if (show_stats()) { alloc_hook(b, size()); }
    if (b) {
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
    if (show_stats()) { alloc_hook(b, size()); }
    if (b) {
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
    if (show_stats()) { alloc_hook(b, size()); }
    if (b) {
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
    if (show_stats()) { alloc_hook(b, size()); }
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
    if (show_stats()) { recycle_hook(b, size()); }
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
 
private:
  vec<ptr<T> > _v;
  size_t _lim;
  bool _alive;
};

//-----------------------------------------------------------------------------

// Recycler for non-refcounted type T
template<class T>
class nonref_recycler_t : public recycler_stats_t {
public:
  nonref_recycler_t (size_t s, str name) : 
      recycler_stats_t(name),
      _lim (s),
      _alive (true)
  { _vp.reserve (s/2); }

  ~nonref_recycler_t () { _alive = false; }
  
  T * alloc ()
  {
    T * ret;
    bool b = can_alloc();
    if (show_stats()) { alloc_hook(b, size()); }
    if (b) {
      ret = _vp.pop_back ();
      ret->init ();
    } else {
      ret = New T ();
    }
    return ret;
  }

  template<class A1>
  T * alloc (const A1 &a1) 
  {
    T * ret;
    bool b = can_alloc();
    if (show_stats()) { alloc_hook(b, size()); }
    if (b) {
      ret = _vp.pop_back ();
      ret->init (a1);
    } else {
      ret = New T (a1);
    }
    return ret;
  }

  template<class A1, class A2>
  T * alloc (const A1 &a1, const A2 &a2) 
  {
    T *ret;
    bool b = can_alloc();
    if (show_stats()) { alloc_hook(b, size()); }
    if (b) {
      ret = _vp.pop_back ();
      ret->init (a1, a2);
    } else {
      ret = New T (a1, a2);
    }
    return ret;
  }

  template<class A1, class A2, class A3>
  T * alloc (const A1 &a1, const A2 &a2, const A3 &a3) 
  {
    T * ret;
    bool b = can_alloc();
    if (show_stats()) { alloc_hook(b, size()); }
    if (b) {
      ret = _vp.pop_back ();
      ret->init (a1, a2, a3);
    } else {
      ret = New T (a1, a2, a3);
    }
  }

  void recycle (T *o)
  {    
    bool b = can_recycle();
    if (show_stats()) { recycle_hook(b, size()); }
    if (b) {
      _vp.push_back (o);
    } else {
      delete o;
    }
  }

  size_t size() { return _vp.size(); }
  bool can_alloc() { return _vp.size(); }
  bool can_recycle() { return _alive && (_vp.size() < _lim); }
  void set_limit(size_t limit) { _lim = limit; }

private:
  vec<T *> _vp;
  size_t _lim;
  bool _alive;
};

#endif /* _LIBPUB_PRECYCLE_H_ */
