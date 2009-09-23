// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _LIBPUB_HOLDTAB_H
#define _LIBPUB_HOLDTAB_H

#include "arpc.h"
#include "vec.h"
#include "qhash.h"

template<class C, class A>
class holdvec_t : public vec<C> {
public:
  holdvec_t () {}
  ~holdvec_t () { vec<C>::clear (); }

  void finish (A a)
  {
    while (vec<C>::size ())
      (* vec<C>::pop_front ()) (a);
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
