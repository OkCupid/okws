

// -*-c++-*-
/* $Id$ */

#ifndef _LIBPUB_HOLDTAB_H
#define _LIBPUB_HOLDTAB_H

#include "arpc.h"
#include "vec.h"
#include "qhash.h"

template<class C, class A>
class holdvec_t : public vec<C> {
public:
  holdvec_t () {}
  ~holdvec_t () { clear (); }

  void finish (A a)
  {
    while (size ())
      (* pop_front ()) (a);
  }
};

template<class K, class C, class A>
class holdtab_t {
public:
  holdtab_t () {}
  ~holdtab_t () {}

  bool inq (K k, C c)
  {
    holdvec_t<C,A> **qp = tab[k];
    if (qp) {
      (*qp)->push_back (c);
      return true;
    } else {
      holdvec_t<C,A> *q = New holdvec_t<C,A> ();
      q->push_back (c);
      tab.insert (k, q);
      return false;
    }
  }
  
  void finish (K k, A a)
  {
    holdvec_t<C,A> **qp = tab[k];
    assert (qp);
    holdvec_t<C,A> *q = *qp;
    tab.remove (k);
    q->finish (a);
    delete q;
  }
private:
  qhash<K, holdvec_t<C,A> *> tab;

};

#endif
