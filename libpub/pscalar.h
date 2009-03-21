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

#ifndef _LIBPUB_PSCALAR_H
#define _LIBPUB_PSCALAR_H 1

#include "okws_sfs.h"
#include "vec.h"
#include "qhash.h"
#include "clist.h"
#include "pubutil.h"
#include "arpc.h"
#include "puberr.h"
#include "holdtab.h"
#include "zstr.h"
#include "tame.h"

//-----------------------------------------------------------------------

bool convertdouble (const str &x, double *dp);

//-----------------------------------------------------------------------

class scalar_obj_t {
private:

  //-----------------------------------------------------------------------

  class _p_t {
  public:
    _p_t ();
    _p_t (const str &s);

    int to_int () const;
    int64_t to_int64 () const;
    u_int64_t to_uint64 () const;
    str to_str () const;
    str to_str_n () const { return _s; }
    double to_double () const;
    bool to_bool () const;
    bool is_null () const { return !_s; }

    bool to_int64 (int64_t *out) const;
    bool to_double (double *out) const;
    bool to_uint64 (u_int64_t *out) const;

    void set (const str &s);
    void set (double d);
    void set (int64_t i) { set_i (i); }
    void set_u (u_int64_t u);
    void set_i (int64_t i);
    void clear ();
    
    typedef enum { CNV_NONE = 0, CNV_OK = 1, CNV_BAD = 2 } cnv_status_t;
    
  private:
    str _s;
    
    mutable cnv_status_t _double_cnv, _int_cnv, _uint_cnv;
    mutable double _d;
    mutable int64_t _i;
    mutable u_int64_t _u;
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
  str trim () const;
  str to_str_n () const { return _p->to_str_n (); }

  void set (const str &s) { _p->set (s); }
  void set (double d) { _p->set (d); }
  void set (int64_t i) { _p->set (i); }
  void set_u (u_int64_t u) { _p->set_u (u); }
  void set_i (int64_t i) { _p->set_i (i); }

  void add (const char *c, size_t l);
  void add (const str &s);
  void freeze ();
  bool is_frozen () const { return _frozen; }

  bool operator== (const scalar_obj_t &o2) const;
  
  virtual bool strip_add (const char *s, int l) const { return true; }

private:
  void ready_append ();
  ptr<strbuf> _b;
  ptr<_p_t> _p;
  bool _frozen;
};

//-----------------------------------------------------------------------

bool convertuint (const str &s, u_int64_t *out);

//-----------------------------------------------------------------------

#endif /* _LIBPUB_PSCALAR_H_ */
