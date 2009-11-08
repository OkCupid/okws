// -*-c++-*-
/* $Id: pub.h 3217 2008-03-27 06:18:34Z max $ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#pragma once

#include "okws_sfs.h"
#include "vec.h"
#include "qhash.h"
#include "pubutil.h"
#include "arpc.h"
#include "zstr.h"
#include "tame.h"

//-----------------------------------------------------------------------

bool convertdouble (const str &x, double *dp);
double convertdouble (const str &x);

//-----------------------------------------------------------------------

class scalar_obj_t {
public:
  typedef enum { TYPE_NONE = 0, TYPE_STR = 1, TYPE_INT = 2, 
		 TYPE_UINT = 3, TYPE_DOUBLE = 4, TYPE_INF = 5 } type_t;

private:

  //-----------------------------------------------------------------------

  class _p_t {
  public:
    _p_t ();
    _p_t (const str &s);
    
    typedef enum { CNV_NONE = 0, CNV_OK = 1, CNV_BAD = 2 } cnv_status_t;

    int to_int () const;
    int64_t to_int64 () const;
    u_int64_t to_uint64 () const;
    str to_str () const;
    str to_str_n () const { return _s; }
    double to_double () const;
    bool to_bool () const;
    bool is_null () const { return !_s; }
    bool is_explicit_double () const;
    bool is_explicit_int () const;

    bool to_int64 (int64_t *out) const;
    bool to_double (double *out) const;
    bool to_uint64 (u_int64_t *out) const;

    void set (const str &s);
    void set (double d);
    void set (int64_t i) { set_i (i); }
    void set_u (u_int64_t u);
    void set_i (int64_t i);
    void clear ();
    type_t natural_type () const;
    void set_inf ();
    
  private:
    str _s;
    
    mutable cnv_status_t _double_cnv, _int_cnv, _uint_cnv;
    mutable double _d;
    mutable int64_t _i;
    mutable u_int64_t _u;
    mutable type_t _natural_type;
  };

  //-----------------------------------------------------------------------

public:
  scalar_obj_t ();
  scalar_obj_t (const str &s);
  virtual ~scalar_obj_t () {}
  operator str () const { return to_str (); }
  operator bool () const { return to_bool (); }
  operator int64_t () const { return to_int64 (); }
  operator double () const { return to_double (); }
  operator int() const { return to_int (); }

  bool to_int64 (int64_t *o) const { return _p->to_int64 (o); }
  bool to_double (double *o) const { return _p->to_double (o); }
  int64_t to_int64 () const { return _p->to_int64 (); }
  u_int64_t to_uint64 () const { return _p->to_uint64 (); }
  bool to_uint64 (u_int64_t *o) const { return _p->to_uint64 (o); }
  int to_int () const { return _p->to_int (); }
  str to_str () const { return _p->to_str (); }
  double to_double () const { return _p->to_double (); }
  bool to_bool () const { return _p->to_bool (); }
  bool is_null () const { return _p->is_null (); }
  bool is_explicit_double () const { return _p->is_explicit_double (); }
  str trim () const;
  str to_str_n () const { return _p->to_str_n (); }
  type_t natural_type () const { return _p->natural_type (); }

  void set (const str &s) { _p->set (s); }
  void set (double d) { _p->set (d); }
  void set (int64_t i) { _p->set (i); }
  void set_u (u_int64_t u) { _p->set_u (u); }
  void set_i (int64_t i) { _p->set_i (i); }
  void set_inf () { _p->set_inf (); }
  bool is_inf () const { return natural_type () == TYPE_INF; }

  void add (const char *c, size_t l);
  void add (const str &s);
  void freeze ();
  bool is_frozen () const { return _frozen; }
  int cmp (const scalar_obj_t &o) const;

  bool operator== (const scalar_obj_t &o2) const { return cmp (o2) == 0; }
  bool operator!= (const scalar_obj_t &o2) const { return cmp (o2) != 0; }
  bool operator<= (const scalar_obj_t &o2) const { return cmp (o2) <= 0; }
  bool operator>= (const scalar_obj_t &o2) const { return cmp (o2) >= 0; }
  bool operator<  (const scalar_obj_t &o2) const { return cmp (o2) <  0; }
  bool operator>  (const scalar_obj_t &o2) const { return cmp (o2) >  0; }

  scalar_obj_t operator+ (const scalar_obj_t &s) const;
  scalar_obj_t operator- (const scalar_obj_t &s) const;
  scalar_obj_t operator* (const scalar_obj_t &s) const;
  scalar_obj_t operator/ (const scalar_obj_t &s) const;
  scalar_obj_t operator% (const scalar_obj_t &s) const;
  
  virtual bool strip_add (const char *s, int l) const { return true; }

private:
  scalar_obj_t div_or_mod (const scalar_obj_t &s, bool div) const;
  void ready_append ();
  ptr<strbuf> _b;
  ptr<_p_t> _p;
  bool _frozen;
};

//-----------------------------------------------------------------------

bool convertuint (const str &s, u_int64_t *out);

//-----------------------------------------------------------------------

