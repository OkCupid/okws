// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

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

#ifndef _LIBOKXML_SMARTVEC_H
#define _LIBOKXML_SMARTVEC_H

template<class T, size_t N = 0>
class smart_vec_t : vec<T,N>
{ 
public:
  smart_vec_t () : vec<T,N> (), _pops (0) {}

  typedef typename vec<T,N>::elm_t elm_t;
  elm_t pop_front () { _pops ++; return vec<T,N>::pop_front (); }
  void popn_front (size_t s) { _pops += s; vec<T,N>::popn_front (s); }

  class slot {
  public:
    slot (smart_vec_t<T,N> &v, ptrdiff_t i) : _vec (v), _i (i) {}
    operator elm_t () const { return _vec[pos ()]; }
    const slot &operator=(T t) const { _vec[pos ()] = t; return *this; }

    bool valid () const 
    { return pos () >= 0 && pos () < ptrdiff_t (_vec.size ()); }
  private:
    ptrdiff_t pos () const { return _i - _vec.pops (); }
    smart_vec_t<T,N> &_vec;
    ptrdiff_t _i;
  };
		   
  class const_slot {
  public:
    const_slot (const smart_vec_t<T,N> &v, ptrdiff_t i) : _vec (v), _i (i) {}
    operator elm_t () const { return _vec[pos ()]; }

    bool valid () const 
    { return pos () >= 0 && pos () < ptrdiff_t (_vec.size ()); }
  private:
    ptrdiff_t pos () const { return _i - _vec.pops (); }
    const smart_vec_t<T,N> &_vec;
    ptrdiff_t _i;
  };

  slot operator[] (ptrdiff_t i) { return slot (*this, i); }
  const_slot operator[] (ptrdiff_t i) const { return const_slot (*this, i); }
  ptrdiff_t pops () const { return _pops; }
private:
  ptrdiff_t _pops;
};

#endif /* _LIBOKXML_SMARTVEC_H */
