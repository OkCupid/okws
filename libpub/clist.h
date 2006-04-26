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

#ifndef _CLIST_H_INCLUDED
#define _CLIST_H_INCLUDED

#include "list.h"

template<class T>
struct clist_entry {
  T *next;
  T *prev;
};

template<class T, clist_entry<T> T::*field>
struct clist_t
{
  T *first;
  T *last;
  u_int nelem;

  clist_t () : nelem (0) { first = last = NULL; }

  bool is_empty () const { return (first == NULL); }

  u_int size () const { return nelem; }

  void deleteall (bool del = true)
  {
    T *n;
    for (T *e = first; e; e = n) {
      n = next (e);
      if (del) delete e;
    }
    first = last = NULL;
    nelem = 0;
  }

  void clear () { deleteall (false); }

  void insert_head (T *elm) {
    if (((elm->*field).next = first))
      (first->*field).prev = elm;
    else
      last = elm;
    first = elm;
    (elm->*field).prev = NULL;
    nelem ++;
  }

  void insert_tail (T *elm) {
    (elm->*field).next = NULL;
    (elm->*field).prev = last;
    if (last)
      (last->*field).next = elm;
    else
      first = elm;
    last = elm;
    nelem ++;
  }

  static T *next (T *elm) {
    return (elm->*field).next;
  }

  static const T *cnext (const T *elm) {
    return (elm->*field).next;
  }
  
  static T *prev (T *elm) {
    return (elm->*field).prev;
  }

  T *remove (T *elm) {
    if ((elm->*field).next)
      ((elm->*field).next->*field).prev = (elm->*field).prev;
    else 
      last = (elm->*field).prev;
    if ((elm->*field).prev)
      ((elm->*field).prev->*field).next = (elm->*field).next;
    else
      first = (elm->*field).next;
    nelem --;
    return elm;
  }

  T *pop_front ()
  {
    if (first) {
      T *el = first;
      remove (first);
      return el;
    } else 
      return NULL;
  }

  T *pop_tail ()
  {
    if (last) {
      T *el = last;
      remove (last);
      return el;
    } else
      return NULL;
  }

  void replace (T *what, T *with) 
  {
    (with->*field).prev = (what->*field).prev;
    (with->*field).next = (what->*field).next;

    if ((with->*field).prev) 
      ((with->*field).prev->*field).next = with;
    else
      first = with;

    if ((with->*field).next)
      ((with->*field).next->*field).prev = with;
    else
      last = with;
  }

      
  void append_list (clist_t<T,field> *l)
  {
    // take care of empty lists first
    nelem += l->size ();
    if (!l->first)
      return;
    if (!first) {
      *this = *l;
      return;
    }

    (last->*field).next = l->first;
    (l->first->*field).prev = last;

    last = l->last;
    l->first = first;

  }
  void traverse (typename callback<void, T *>::ref cb) const {
    T *p, *np;
    for (p = first; p; p = np) {
      np = (p->*field).next;
      (*cb) (p);
    }
  }
};

#endif /* _CLIST_H_INCLUDED */

