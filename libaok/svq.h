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

#ifndef _LIBAOK_SVQ_H
#define _LIBAOK_SVQ_H

#include "arpc.h"
#include "vec.h"
#include "qhash.h"

template<class R>
class svvec_t : public vec<svccb *> {
public:
  svvec_t () {}
  ~svvec_t () { finish (); }
  
  void finish () 
  {
    while (size ())
      pop_front ()->reject (PROC_UNAVAIL);
  }
  
  void finish (R r) 
  {
    while (size ())
      pop_front ()->reply (r);
  }

};

template<class R>
class svq_t {
public:
  svq_t () : bflag (false) {}
  ~svq_t () {}

  bool blocked (svccb *s)
  {
    v.push_back (s);
    if (bflag) {
      return true;
    } else {
      bflag = true;
      return false;
    }
  }

  void finish (R r)
  {
    bflag = false;
    v.finish (r);
  }

  bool blocked () const { return bflag; }

protected:
  bool bflag;
private:
  svvec_t<R> v;
};

template<class K, class R>
class svqtab_t {
public:
  svqtab_t () {}
  ~svqtab_t () {}

  bool inq (K k, svccb *s)
  {
    svvec_t<R> **qp = tab[k];
    if (qp) {
      (*qp)->push_back (s);
      return true;
    } else {
      svvec_t<R> *q = New svvec_t<R> ();
      q->push_back (s);
      tab.insert (k, q);
      return false;
    }
  }

  void finish (K k, R r)
  {
    svvec_t<R> **qp = tab[k];
    assert (qp);
    svvec_t<R> *q = *qp;
    tab.remove (k);
    q->finish (r);
    delete q;
  }

private:
  qhash<K, svvec_t<R> *> tab;
};


#endif /* _LIBAOK_SVQ_H */
