// -*-c++-*-
/* $Id$ */

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


