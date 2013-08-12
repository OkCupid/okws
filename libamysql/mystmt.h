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

#ifndef _LIBAMYSQL_STMTS_H
#define _LIBAMYSQL_STMTS_H

#include "amysql.h"
#include <vector>

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#include <functional>
#endif

typedef enum { 
    AMYSQL_NONE = 0, AMYSQL_EXEC = 1, AMYSQL_FETCH = 2,
    AMYSQL_FETCH_DONE = 3
} amysql_state_t;

class mystmt_t {
public:
  mystmt_t (tz_corrector_t *t) : 
      errno_n (0), state (AMYSQL_NONE), lqt (0), _tzc (t),
      _tz_run (false) {}
  virtual ~mystmt_t () {};
  virtual adb_status_t fetch2 (vec<mybind_res_t> &arr, bool bnd = false) = 0;
  str error () const { return err; }
  unsigned int errnum () const { return errno_n; }
  bool execute () { 
     vec<mybind_param_t> args;
     return execute1 ((MYSQL_BIND *)NULL, args);
  }
  bool execute_argvec (const amysql_scalars_t &args);
  bool execute_argvec (vec<mybind_param_t> &args);
  adb_status_t fetch_argvec (vec<amysql_scalars_t> *row, size_t nf);
  adb_status_t fetch_argvec (vec<mybind_res_t> &);
  virtual str get_last_qry () const { return NULL; }
  void set_long_query_timer (u_int m) { lqt = m ;}
  virtual const MYSQL_FIELD *fetch_fields (size_t *n) = 0;
  virtual size_t n_rows () const { return 0; }
  virtual size_t affected_rows () const = 0;
  virtual u_int64_t insert_id () = 0;
protected:
  virtual bool execute2 (MYSQL_BIND *b, vec<mybind_param_t> &arr) = 0;
  bool execute1 (MYSQL_BIND *b, vec<mybind_param_t> &arr);
  virtual str dump (vec<mybind_param_t> &arr) = 0;

  str err;
  unsigned int errno_n;
  amysql_state_t state;
  u_int lqt;  // long query timer (in milliseconds)
  tz_corrector_t *_tzc;
  bool _tz_run;

public:

#ifdef __GXX_EXPERIMENTAL_CXX0X__

  template <typename... Args>
  adb_status_t fetch (Args &&...args) {
    vec<mybind_res_t> arr{ std::forward<Args>(args)... };
    return fetch2 (arr, true);
  }

  template <typename... Args>
  bool execute (Args &&...args) {
    MYSQL_BIND bnd[sizeof...(Args)];
    vec<mybind_param_t> arr{ std::forward<Args>(args)... };
    return execute1 (bnd, arr);
  }

#else
// The C++11-ity with which you build okws must match that of your project.
// Right now mystmt_ag.pl has #ifdef guards around the old functions.
#include "mystmt_ag.h"
#endif
};

typedef ptr<mystmt_t> sth_t;

class sth_parsed_t : public mystmt_t 
{
public:
  sth_parsed_t (MYSQL *m, const str &q, u_int o = 0, tz_corrector_t *t = NULL)
    : mystmt_t (t), mysql (m), qry (q), n_iparam (0), bufs (NULL), opts (o),
      myres (NULL) {}
  ~sth_parsed_t ();
  static ptr<sth_parsed_t> 
  alloc (MYSQL *m, const str &q, u_int o, tz_corrector_t *tzc)
  { return New refcounted<sth_parsed_t> (m, q, o, tzc); }
  bool parse ();
  adb_status_t fetch2 (vec<mybind_res_t> &arr, bool bnd = false);
  str get_last_qry () const { return last_qry; }
  const MYSQL_FIELD *fetch_fields (size_t *n);
  size_t n_rows () const;
  size_t affected_rows () const;
  u_int64_t insert_id ();
protected:
  bool execute2 (MYSQL_BIND *b, vec<mybind_param_t> &arr);
  str dump (vec<mybind_param_t> &arr);
  str make_query (vec<mybind_param_t> &arr);
  
  void dealloc_bufs ();
  void alloc_bufs ();
  void clearfetch ();

  MYSQL *mysql;
  const str qry;
  u_int n_iparam;
  vec<str> qry_parts;
  char **bufs;
  u_int *lens;
  u_int opts;
  MYSQL_RES *myres;
  unsigned long *length_arr;
  size_t my_res_n;
  str last_qry;
};

#if defined(HAVE_MYSQL_BIND) && defined(HAVE_MYSQL_BINDFUNCS)
class sth_prepared_t : public mystmt_t 
{
public:
  sth_prepared_t (MYSQL_STMT *s, const str &q, u_int o = 0, 
		  tz_corrector_t *t = NULL) 
    : mystmt_t (t), sth (s), bnds (NULL), qry (q), opts (o) {}
  ~sth_prepared_t ();
  static ptr<sth_prepared_t> 
  alloc (MYSQL_STMT *s, const str &q, u_int o, tz_corrector_t *tzc)
  { return New refcounted<sth_prepared_t> (s, q, o, tzc); }
  adb_status_t fetch2 (vec<mybind_res_t> &arr, bool bnd = false);
  const MYSQL_FIELD *fetch_fields (size_t *n) { return NULL; }
  size_t affected_rows () const;
  u_int64_t insert_id ();
protected:
  bool execute2 (MYSQL_BIND *b, vec<mybind_param_t> &arr);
  str dump (vec<mybind_param_t> &arr);
  void bind (MYSQL_BIND *b, vec<mybind_param_t> &arr);
  bool bind_result (vec<mybind_res_t> &arr);
  void clearfetch ();
  MYSQL_STMT *sth;
  MYSQL_BIND *bnds;
  const str qry;
  u_int opts;
};
#endif


#endif /* _LIBAMYSQL_STMTS_H */
