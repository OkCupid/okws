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

#ifndef _FHASH_H_INCLUDED
#define _FHASH_H_INCLUDED

#include "ihash.h"

template<class V, class K>
struct keyfn {
  keyfn () {}
  const K & operator() (V *v) const { K k = new K (); return *k; }
};

template<class K, class V, ihash_entry<V> V::*field,
	 class F = keyfn<V, K>, class H = hashfn<K>, class E = equals<K> >
class fhash
  : public ihash_core <V, field>
{
  const E eq;
  const H hash;
  const F keyfn;

public:
  fhash () {}
  fhash (const E &e, const H &h, const F &k) : eq (e), hash (h), keyfn (k) {}

  void insert (V *elm) { insert_val (elm, hash (keyfn (elm))); }

  V *operator[] (const K &k) const {
    V *v;
    for (v = lookup_val (hash (k)); 
	 v && !eq (k, keyfn (v));
	 v = next_val (v)) 
    ;
    return v;
  }
  V *nextkeq (V *v) {
    const K &k = keyfn (v);
    while ((v = next_val (v)) && !eq (k, keyfn (v)))
    ;
    return v;
  };

};

#endif /* _FHASH_H_INCLUDED */


