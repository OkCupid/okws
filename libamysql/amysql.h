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

#ifndef _LIBAMYSQL_AMYSQL_H
#define _LIBAMYSQL_AMYSQL_H

#include "web.h"
#include "okwsconf.h"
#include "okws_sfs.h"
#include "mybind.h"
#include "amt.h"

//-----------------------------------------------------------------------

#define AMYSQL_NOCACHE     (1 << 0)  /* cache prepared querieds in hashtab */
#define AMYSQL_PREPARED    (1 << 1)  /* use prepared statement handles */
#define AMYSQL_DEFAULT     (1 << 2)  /* use global object defaults */
#define AMYSQL_USERES      (1 << 3)  /* call use res as opposed to store res */
#define AMYSQL_NOTZCORRECT (1 << 4)  /* turn of TZ correction */

//-----------------------------------------------------------------------

typedef enum {
  FETCH_OK = 0,
  FETCH_ERROR = 1, 
  FETCH_NO_DATA = 2,
  FETCH_BIND_ERROR = 3
} fetch_status_t;

//-----------------------------------------------------------------------

class tz_corrector_t;

//-----------------------------------------------------------------------
class mystmt_t;
class mysql_t {
public:
  mysql_t (u_int o = 0) : opts (o), errcode (ADB_OK) { mysql_init (&mysql); }
  ~mysql_t () { mysql_close (&mysql); }
  ptr<mysql_t> alloc () { return New refcounted<mysql_t> (); }
  bool connect (const dbparam_t &p);
  bool connect (const str &d, const str &u = NULL, const str &h = NULL, 
		const str &pw = NULL, u_int prt = 0, u_long fl = 0);
  str error () const { return err; }
  adb_status_t error_code () const { return errcode; }
  ptr<mystmt_t> prepare (const str &q, u_int opts = AMYSQL_DEFAULT,
			 tz_corrector_t *tzc = NULL);
  u_int64_t insert_id () { return mysql_insert_id (&mysql); }
  u_int64_t affected_rows() { return mysql_affected_rows(&mysql);}
#ifdef HAVE_MYSQL_BIND
  u_int64_t warning_count() { return mysql_warning_count(&mysql);}
#endif
  
  u_int opts;
  str err;
  adb_status_t errcode;
  MYSQL mysql;
  qhash<str, ptr<mystmt_t> > cache;
};

//-----------------------------------------------------------------------

class amysql_thread_guts_t {
public:
  amysql_thread_guts_t (u_int o);
  virtual ~amysql_thread_guts_t () {}
  ptr<mystmt_t> prepare (const str &q, u_int opts = AMYSQL_DEFAULT,
			 mysql_t *m = NULL);
  bool init_phase0 ();
  virtual bool is_readied () const = 0;
  virtual int getid () const = 0;
  mysql_t *get_mysql () { return &mysql; }
  str error () const { return _err; }
  adb_status_t error_code () const { return _errcode; }
  void set_safe (bool b) { _safe = b; }
  bool is_safe () const { return _safe; }
protected:
  mysql_t mysql;
  tz_corrector_t *_tzc;

  // copy the tid parameter from a sibling class (mtd_thread_t),
  // so that TWARN() still works.  Sorry for this hack.
  int tid;

  str _err;
  adb_status_t _errcode;
  bool _safe;
};

//-----------------------------------------------------------------------

class amysql_thread_t : public mtd_thread_t, public amysql_thread_guts_t {
public:
  amysql_thread_t (mtd_thread_arg_t *a, u_int o = 0)
    : mtd_thread_t (a),
      amysql_thread_guts_t (o) {}
  bool is_readied () const { return mtd_thread_t::readied; }
  int getid () const { return mtd_thread_t::getid (); }
  virtual ~amysql_thread_t () {}
  bool init_phase0 () { return amysql_thread_guts_t::init_phase0 (); }
};

//-----------------------------------------------------------------------

class amysql_thread2_t : public amt::thread2_t, public amysql_thread_guts_t {
public:
  amysql_thread2_t (mtd_thread_arg_t *a, u_int o = 0)
    : amt::thread2_t (a),
      amysql_thread_guts_t (o) {}
  bool is_readied () const { return amt::thread2_t::readied; }
  int getid () const { return amt::thread2_t::getid (); }
  virtual ~amysql_thread2_t () {}
  bool init_phase0 () { return amysql_thread_guts_t::init_phase0 (); }
};

//-----------------------------------------------------------------------

class tz_corrector_t {
public:
  tz_corrector_t (amysql_thread_guts_t *t) 
    : _thr (t), _nxt_update (0), _gmtoff (0), 
      _fetching (false) {}
  long gmt_offset () ;
  bool prepare ();
  bool fetching () const { return _fetching; }
  int getid () const { return _thr->getid (); }

private:
  bool run ();
  void reschedule ();
  time_t next_hour (time_t t);
  amysql_thread_guts_t *_thr;
  time_t _nxt_update;
  long _gmtoff;
  ptr<mystmt_t> _sth;
  bool _fetching;
};

//-----------------------------------------------------------------------

#define PREP(x) prepare (strbuf () << x)

//-----------------------------------------------------------------------

#endif /* _LIBAMYSQL_AMYSQL_H */
