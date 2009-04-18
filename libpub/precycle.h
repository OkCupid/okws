// -*-c++-*-
/* $Id: precycle.h 2784 2007-04-20 16:32:00Z max $ */

#ifndef _LIBPUB_PRECYCLE_H_
#define _LIBPUB_PRECYCLE_H_

#include "vec.h"

template<class T>
class recycler_t {
public:
  recycler_t (size_t s) : _lim (s), _alive (true) { _v.reserve (s/2); }
  ~recycler_t () { _alive = false; }
  
  ptr<T> alloc ()
  {
    ptr<T> ret;
    if (_v.size ()) {
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
    if (_v.size ()) {
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
    if (_v.size ()) {
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
    if (_v.size ()) {
      ret = _v.pop_back ();
      ret->init (a1, a2, a3);
    } else {
      ret = New refcounted<T> (a1, a2, a3);
    }
  }

  void recycle (T *o)
  {
    if (_alive && _v.size () < _lim) {
      _v.push_back (mkref (o));
    }
  }

  vec<ptr<T> > _v;
  size_t _lim;
  bool _alive;
};



#endif /* _LIBPUB_PRECYCLE_H_ */
