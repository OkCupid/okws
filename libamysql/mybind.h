// -*-c++-*-
/* $Id: web.h 5098 2010-01-21 02:51:43Z max $ */

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

#pragma once

#include "okwsconf.h"
#include "okws_sfs.h"

#ifdef HAVE_MYSQL
#include "mysql.h"
#include "web_prot.h"
#include "web.h"
#include "qhash.h"
#include "parseopt.h"
#include "pub.h"

//=======================================================================

// hack a workaround mysql compatibility changes
#ifndef HAVE_MYSQL_BIND

typedef char MYSQL_BIND;
#define MYSQL_TYPE_DECIMAL     FIELD_TYPE_DECIMAL
#define MYSQL_TYPE_TINY        FIELD_TYPE_TINY
#define MYSQL_TYPE_SHORT       FIELD_TYPE_SHORT
#define MYSQL_TYPE_LONG        FIELD_TYPE_LONG
#define MYSQL_TYPE_FLOAT       FIELD_TYPE_FLOAT
#define MYSQL_TYPE_DOUBLE      FIELD_TYPE_DOUBLE
#define MYSQL_TYPE_NULL        FIELD_TYPE_NULL
#define MYSQL_TYPE_TIMESTAMP   FIELD_TYPE_TIMESTAMP
#define MYSQL_TYPE_LONGLONG    FIELD_TYPE_LONGLONG
#define MYSQL_TYPE_INT24       FIELD_TYPE_INT24
#define MYSQL_TYPE_DATE        FIELD_TYPE_DATE
#define MYSQL_TYPE_TIME        FIELD_TYPE_TIME
#define MYSQL_TYPE_DATETIME    FIELD_TYPE_DATETIME
#define MYSQL_TYPE_YEAR        FIELD_TYPE_YEAR
#define MYSQL_TYPE_NEWDATE     FIELD_TYPE_NEWDATE
#define MYSQL_TYPE_ENUM        FIELD_TYPE_ENUM
#define MYSQL_TYPE_SET         FIELD_TYPE_SET
#define MYSQL_TYPE_TINY_BLOB   FIELD_TYPE_TINY_BLOB
#define MYSQL_TYPE_MEDIUM_BLOB FIELD_TYPE_MEDIUM_BLOB
#define MYSQL_TYPE_LONG_BLOB   FIELD_TYPE_LONG_BLOB
#define MYSQL_TYPE_BLOB        FIELD_TYPE_BLOB
#define MYSQL_TYPE_VAR_STRING  FIELD_TYPE_VAR_STRING
#define MYSQL_TYPE_STRING      FIELD_TYPE_STRING
#define MYSQL_TYPE_CHAR        FIELD_TYPE_TINY
#define MYSQL_TYPE_INTERVAL    FIELD_TYPE_ENUM
#define MYSQL_TYPE_GEOMETRY    FIELD_TYPE_GEOMETRY

#endif

//=======================================================================

#define BIND_BUF_MIN 256
#define MYSQL_DATE_STRLEN 20     // strlen ("YYYY-MM-DD HH:MM:SS")

//=======================================================================

typedef enum enum_field_types eft_t;

//=======================================================================

class mybind_t {
public:
  mybind_t (eft_t t) : ft (t), nullfl (0) {}
  virtual ~mybind_t () {}
#ifdef HAVE_MYSQL_BIND
  virtual void bind (MYSQL_BIND *bind, bool param) = 0;
#endif
  virtual void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l) = 0;
  virtual bool isnull () const { return nullfl; }
  virtual void assign () {}
  virtual bool read_str (const char *c, unsigned long l, eft_t typ) = 0;
  virtual str to_str () const = 0;
  virtual bool is_xdr_union_type () const { return false; }
protected:
  eft_t ft;
  my_bool nullfl;
};

//=======================================================================

#ifdef HAVE_MYSQL_BIND
void mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_t *x);
void mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_date_t *x);
void mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_time_t *x);
#endif

//=======================================================================

class mybind_date_xassign_t {
public:
  mybind_date_xassign_t () {}
#ifdef HAVE_MYSQL_BIND
  virtual void assign (const MYSQL_TIME &tm, okdatep_t d) = 0;
#endif
  virtual void parsed_assign (okdatep_t d) = 0;
  virtual ~mybind_date_xassign_t () {}
};

//-----------------------------------------------------------------------

template<class C> // MyBind Date Xassign Template 
class mbdxat_t : public mybind_date_xassign_t {
public:
  mbdxat_t (C *p) : mybind_date_xassign_t (), pntr (p) {}
#ifdef HAVE_MYSQL_BIND
  void assign (const MYSQL_TIME &tm, okdatep_t d);
#endif

  // special "assign" function for parsed queries; usually
  // not need (can be handled by read_str, which parses
  // responses
  void parsed_assign (okdatep_t d) { d->to_xdr (pntr); }
private:
  C *pntr;
};

//-----------------------------------------------------------------------

class mybind_date_t : public mybind_t {
public:
  mybind_date_t (okdatep_t d);

  mybind_date_t (const x_okdate_t &d);
  mybind_date_t (const okdate_t &d);
  mybind_date_t (const x_okdate_time_t &t) 
    : mybind_t (MYSQL_TYPE_TIME), datep (okdate_t::alloc (t)), pntr (NULL),
      parsed (false), palloced (false) {}
  mybind_date_t (const x_okdate_date_t &d) 
    : mybind_t (MYSQL_TYPE_DATE), datep (okdate_t::alloc (d)), pntr (NULL),
      parsed (false), palloced (false) {}

  mybind_date_t (x_okdate_t *d)
    : mybind_t (MYSQL_TYPE_DATETIME), datep (New refcounted<okdate_t> ()), 
       pntr (New mbdxat_t<x_okdate_t> (d)), parsed (false), 
	       palloced (true) {}

 mybind_date_t (x_okdate_time_t *t)
    : mybind_t (MYSQL_TYPE_TIME), 
      datep (New refcounted<okdate_t> ()), 
      pntr (New mbdxat_t<x_okdate_time_t> (t)),
      parsed (false), palloced (true) {}

  mybind_date_t (x_okdate_date_t *d)
    : mybind_t (MYSQL_TYPE_DATE), 
      datep (New refcounted<okdate_t> ()), 
      pntr (New mbdxat_t<x_okdate_date_t> (d)),
      parsed (false), palloced (true) {}

  ~mybind_date_t () { if (palloced && pntr) delete pntr; }
  void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l);
  str to_str () const;

  operator okdatep_t () const 
  { return isnull () ? static_cast<okdatep_t> (NULL) : datep; }
  bool read_str (const char *c, unsigned long l, eft_t typ);

#ifdef HAVE_MYSQL_BIND
  // only call assign as part of binding operations; no need
  // in parsed representation....
  void assign () 
  {
    if (pntr) 
      pntr->assign (tm, datep);
  }

  void bind (MYSQL_BIND *bind, bool param);
private:
  MYSQL_TIME tm;
#endif

private:
  okdatep_t datep;
  mybind_date_xassign_t *pntr;
  u_long len;
  bool parsed;
  bool palloced;
};

//-----------------------------------------------------------------------

class mybind_str_t : public mybind_t {
public:
  mybind_str_t (const str &s, eft_t t = MYSQL_TYPE_STRING, bool hld = false) :
    mybind_t (t), hold (hld ? s : sNULL), size (s.len ()), 
    buf (const_cast<char *> (s.cstr ())), balloced (false),
    pntr (NULL) {}

  mybind_str_t (u_int s = 1024, eft_t t = MYSQL_TYPE_STRING) : 
    mybind_t (t), hold (NULL), size (s), buf (New char[s]), balloced (true),
    pntr (NULL) {}
  
  mybind_str_t (str *p, u_int s = 1024, eft_t t = MYSQL_TYPE_STRING) :
    mybind_t (t), hold (NULL), size (s), buf (New char[s]), balloced (true),
    pntr (p) {}
  
  ~mybind_str_t () { if (balloced) delete [] buf; }
#ifdef HAVE_MYSQL_BIND
  void bind (MYSQL_BIND *bnd, bool param);
#endif
  virtual void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l);
  virtual str to_str  () const;
  virtual void assign () { *pntr = str (buf, len); }
  operator str () const { return isnull () ? sNULL : str (buf, len); }
  bool read_str (const char *c, unsigned long l, eft_t typ)
  { *pntr = str (c, l); return true; }

protected:
  void set (str s);
  str hold;
  u_int size;
  char *buf;
  u_long len;
  bool balloced;
  str *pntr;
};

//-----------------------------------------------------------------------

class mysql_var_t {
public:
  mysql_var_t (str s) : _val (s) {}
  str val () const { return _val; }
  str _val;
};

//-----------------------------------------------------------------------

class mybind_var_t :  public mybind_t {
public:
  mybind_var_t (str s) 
    : mybind_t (MYSQL_TYPE_STRING), _val (s) { assert (s); }
  mybind_var_t (const mysql_var_t &v) 
    : mybind_t (MYSQL_TYPE_STRING), _val (v.val ()) { assert (_val); }
#ifdef HAVE_MYSQL_BIND
  void bind (MYSQL_BIND *bind, bool param) {}
#endif
  void to_qry (MYSQL *m, strbuf *b, char **ss, u_int *l);
  str to_str () const { return _val; }
  void assign () {}
  bool read_str (const char *c, unsigned long l, eft_t typ) { return false; }
protected:
  str _val;
};

//-----------------------------------------------------------------------

template<size_t n>
class mybind_rpcstr_t :  public mybind_t {
public:
  mybind_rpcstr_t (rpc_bytes<n> *rr) 
    : mybind_t (MYSQL_TYPE_STRING), pntr (rr), mys (&s) {}
  mybind_rpcstr_t (const rpc_bytes<n> &in)
    : mybind_t (MYSQL_TYPE_STRING), 
      mys (str (in.base (), in.size ()), MYSQL_TYPE_STRING, true) {}
#ifdef HAVE_MYSQL_BIND
  void bind (MYSQL_BIND *bind, bool param) { mys.bind (bind, param); }
#endif
  void to_qry (MYSQL *m, strbuf *b, char **ss, u_int *l)
  { mys.to_qry (m, b, ss, l); }
  str to_str () const { return mys.to_str (); }
  void assign () { mys.assign (); *pntr = s; }
  bool read_str (const char *c, unsigned long l, eft_t typ)
  {
    if (!mys.read_str (c, l, typ)) return false;
    *pntr = s;
    return true;
  }
protected:
  rpc_bytes<n> *pntr;
  str s;
  mybind_str_t mys;
};

//-----------------------------------------------------------------------

class mybind_json_t : public mybind_t {
public:
  mybind_json_t () : mybind_t (MYSQL_TYPE_NULL), _x (XPUB3_JSON_NULL),
		     _pntr (NULL) {}
  mybind_json_t (const xpub3_json_t &s) 
    : mybind_t (MYSQL_TYPE_NULL), _x (s) , _pntr (&_x) {}
  mybind_json_t (xpub3_json_t *x)
    : mybind_t (MYSQL_TYPE_NULL), _pntr (x) {}
  virtual ~mybind_json_t () {}
  bool read_str (const char *c, unsigned long l, eft_t typ);
  void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l);
  bool is_xdr_union_type () const { return true; }
  virtual str to_str () const;
  virtual void bind (MYSQL_BIND *bind, bool param) {}
  static ptr<mybind_t> alloc (const xpub3_json_t &x, bool lenient);
  xpub3_json_t _x, *_pntr;
};

//-----------------------------------------------------------------------

class mybind_xdr_union_t : public mybind_t {
public:
  mybind_xdr_union_t () : mybind_t (MYSQL_TYPE_NULL), _x (AMYSQL_TYPE_NULL),
			  _pntr (NULL) {}
  mybind_xdr_union_t (const amysql_scalar_t &s) 
    : mybind_t (MYSQL_TYPE_NULL), _x (s) , _pntr (&_x) {}
  mybind_xdr_union_t (amysql_scalar_t *x)
    : mybind_t (MYSQL_TYPE_NULL), _pntr (x) {}
  virtual ~mybind_xdr_union_t () {}
  bool read_str (const char *c, unsigned long l, eft_t typ);
  void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l);
  bool is_xdr_union_type () const { return true; }
  virtual str to_str () const;
  virtual void bind (MYSQL_BIND *bind, bool param) {}
  static ptr<mybind_t> alloc (const amysql_scalar_t &x, bool lenient);
  amysql_scalar_t _x, *_pntr;
};

//-----------------------------------------------------------------------

class mybind_double_t : public mybind_t {
public:
  mybind_double_t ()
    : mybind_t (MYSQL_TYPE_DOUBLE), val (0), pntr (NULL) {}
  mybind_double_t (const double& v)
    : mybind_t (MYSQL_TYPE_DOUBLE), val (v), pntr (NULL) {}
  mybind_double_t (double* v)
    : mybind_t (MYSQL_TYPE_DOUBLE), val (0), pntr (v) {}
  ~mybind_double_t () {}
  inline void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l) { *b << val; }
  str to_str () const { strbuf b; b << val; return b; }
  bool read_str (const char *c, unsigned long l, eft_t typ);

#ifdef HAVE_MYSQL_BIND
  virtual void assign () { if (nullfl) val = 0; pntr_assign (); }
  inline void pntr_assign () { if (pntr) *pntr = val; }
  void bind (MYSQL_BIND *bind, bool param);
#endif

private:
  double val;
  double *pntr;
  u_long length;
};

//-----------------------------------------------------------------------

template<typename T, typename M> 
class mybind_num_t : public mybind_t {
public:
  mybind_num_t (eft_t t, T *p) : mybind_t (t), pntr (p), ppntr (NULL) {}
  mybind_num_t (eft_t t) : mybind_t (t), pntr (NULL), ppntr (NULL)  {}
  mybind_num_t (eft_t t, T nn) : mybind_t (t), n (nn), pntr (NULL), 
				 ppntr (NULL) {}
  mybind_num_t (eft_t t, ptr<T> *p) : mybind_t (t), pntr (NULL), ppntr (p) {}
  operator str () const { return isnull () ? NULL : strbuf () << n; }
  virtual void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l) { *b << n; }
  virtual str to_str () const { strbuf b; b << n; return b; }
#ifdef HAVE_MYSQL_BIND
  virtual void bind (MYSQL_BIND *bnd, bool param)
  {
    mysql_n = n;
    bnd->buffer_type = ft;
    bnd->buffer = (char *)&mysql_n;
    bnd->is_null = param ? 0 : &nullfl;
    bnd->length = param ? 0 : &mysql_blows;
    bnd->buffer_length = sizeof (mysql_n);
  }
#endif
  
  inline void pntr_assign ()
  {
    if (pntr)
      *pntr = n;
    if (ppntr) {
      if (nullfl) *ppntr = NULL;
      else *ppntr = New refcounted<T, scalar> (n);
    }
  }

  virtual void assign ()
  {
    if (nullfl)
      mysql_n = 0;
    n = mysql_n;
    pntr_assign ();
  }

  virtual bool read_str (const char *c, unsigned long, eft_t typ)
  {
    if (!c || !convertint (c, &n)) {
      n = 0;
      nullfl = true;
    }
    pntr_assign ();
    return true;
  }

protected:
  T n;
  u_long mysql_blows;
  M mysql_n;
  T *pntr;
  ptr<T> *ppntr;
  str buf;
};

//-----------------------------------------------------------------------

class mybind_u64_t : public mybind_num_t<u_int64_t, unsigned long long> {
public:
  mybind_u64_t (u_int64_t i) :
    mybind_num_t<u_int64_t, unsigned long long> (MYSQL_TYPE_LONGLONG, i) {}
  mybind_u64_t () : 
    mybind_num_t<u_int64_t, unsigned long long> (MYSQL_TYPE_LONGLONG) {}
  mybind_u64_t (u_int64_t *i) :
    mybind_num_t<u_int64_t, unsigned long long> (MYSQL_TYPE_LONGLONG, i) {}
  mybind_u64_t (ptr<u_int64_t> *i) :
    mybind_num_t<u_int64_t, unsigned long long> (MYSQL_TYPE_LONGLONG, i) {}
  operator u_int64_t () const { return isnull () ? 0 : n; }
  virtual bool read_str (const char *c, unsigned long, eft_t typ)
  {
    if (!c || ( (n = strtoull (c, NULL, 10)) == 0 && errno == EINVAL)) {
      n = 0;
      nullfl = true;
    }
    pntr_assign ();
    return true;
  }
};

//-----------------------------------------------------------------------

class mybind_int_t : public mybind_num_t<int, long> {
public:
  mybind_int_t (int i) : mybind_num_t<int, long> (MYSQL_TYPE_LONG, i) {}
  mybind_int_t (int *i) : mybind_num_t<int, long> (MYSQL_TYPE_LONG, i) {}
  mybind_int_t () : mybind_num_t<int, long> (MYSQL_TYPE_LONG) {}
  mybind_int_t (ptr<int> *i) : mybind_num_t<int, long> (MYSQL_TYPE_LONG, i) {}
};

//-----------------------------------------------------------------------

class mybind_time_t : public mybind_num_t<time_t, long> {
public:
  mybind_time_t (time_t t) : mybind_num_t<time_t, long> (MYSQL_TYPE_LONG, t) {}
  mybind_time_t (time_t *t) 
    : mybind_num_t<time_t, long> (MYSQL_TYPE_LONG, t)
  {}
  mybind_time_t () : mybind_num_t<time_t, long> (MYSQL_TYPE_LONG) {}
  mybind_time_t (ptr<time_t> *t) 
    : mybind_num_t<time_t, long> (MYSQL_TYPE_LONG, t) {}
};

//-----------------------------------------------------------------------

class mybind_hyper_t : public mybind_num_t<int64_t, long long> {
public:
  mybind_hyper_t (int64_t i) : 
    mybind_num_t<int64_t, long long> (MYSQL_TYPE_LONGLONG, i) {}
  mybind_hyper_t () : mybind_num_t<int64_t, long long> (MYSQL_TYPE_LONGLONG) {}
  mybind_hyper_t (int64_t *i) : 
    mybind_num_t<int64_t, long long> (MYSQL_TYPE_LONGLONG, i) {}
  mybind_hyper_t (ptr<int64_t> *i) : 
    mybind_num_t<int64_t, long long> (MYSQL_TYPE_LONGLONG, i) {}
  operator int64_t () const { return isnull () ? 0 : n; }
};

//-----------------------------------------------------------------------

class mybind_u32_t : public mybind_num_t<u_int32_t, long long> {
public:
  mybind_u32_t (u_int32_t i) :
    mybind_num_t<u_int32_t, long long> (MYSQL_TYPE_LONGLONG, i) {}
  mybind_u32_t () : mybind_num_t<u_int32_t, long long> (MYSQL_TYPE_LONGLONG) {}
  mybind_u32_t (u_int32_t *i) :
    mybind_num_t<u_int32_t, long long> (MYSQL_TYPE_LONGLONG, i) {}
  mybind_u32_t (ptr<u_int32_t> *i) :
    mybind_num_t<u_int32_t, long long> (MYSQL_TYPE_LONGLONG, i) {}
  operator u_int32_t () const { return isnull () ? 0 : n; }
};

//-----------------------------------------------------------------------

class mybind_short_t : public mybind_num_t<int16_t, short> {
public:
  mybind_short_t (int16_t i) :
    mybind_num_t<int16_t, short> (MYSQL_TYPE_SHORT, i) {}
  mybind_short_t () : mybind_num_t<int16_t, short> (MYSQL_TYPE_SHORT) {}
  mybind_short_t (int16_t *i) :
    mybind_num_t<int16_t, short> (MYSQL_TYPE_SHORT, i) {}
  mybind_short_t (ptr<int16_t> *i) :
    mybind_num_t<int16_t, short> (MYSQL_TYPE_SHORT, i) {}
  operator int16_t () const { return isnull () ? 0 : n ; }
};

//-----------------------------------------------------------------------

class mybind_char_t : public mybind_num_t<char, char> {
public:
  mybind_char_t (char i) :
    mybind_num_t<char, char> (MYSQL_TYPE_TINY, i) {}
  mybind_char_t () : mybind_num_t<char, char> (MYSQL_TYPE_TINY) {}
  mybind_char_t (char *i) :
    mybind_num_t<char, char> (MYSQL_TYPE_TINY, i) {}
  mybind_char_t (ptr<char> *i) :
    mybind_num_t<char, char> (MYSQL_TYPE_TINY, i) {}
  operator char () const { return isnull () ? 0 : n ; }
};

//-----------------------------------------------------------------------

class mybind_u16_t : public mybind_num_t<u_int16_t, long> {
public:
  mybind_u16_t (u_int16_t i) :
    mybind_num_t<u_int16_t, long> (MYSQL_TYPE_LONG, i) {}
  mybind_u16_t () : mybind_num_t<u_int16_t, long> (MYSQL_TYPE_LONG) {}
  mybind_u16_t (u_int16_t *i) :
    mybind_num_t<u_int16_t, long> (MYSQL_TYPE_LONG, i) {}
  mybind_u16_t (ptr<u_int16_t> *i) :
    mybind_num_t<u_int16_t, long> (MYSQL_TYPE_LONG, i) {}
  operator u_int16_t () const { return isnull () ? 0 : n ;}
};

//-----------------------------------------------------------------------

class mybind_u8_t : public mybind_num_t<u_char, short> {
public:
  mybind_u8_t (u_char i) : mybind_num_t<u_char, short> (MYSQL_TYPE_SHORT, i) {}
  mybind_u8_t () : mybind_num_t<u_char, short> (MYSQL_TYPE_SHORT) {}
  mybind_u8_t (u_char *i) 
    : mybind_num_t<u_char, short> (MYSQL_TYPE_SHORT, i) {}
  mybind_u8_t (ptr<u_char> *i) 
    : mybind_num_t<u_char, short> (MYSQL_TYPE_SHORT, i) {}
  operator u_char () const { return isnull () ? 0 : n ; }
};

//-----------------------------------------------------------------------

class mybind_bool_t : public mybind_num_t<bool, bool> {
public:
  mybind_bool_t (bool b)
    : mybind_num_t<bool, bool> (MYSQL_TYPE_TINY, b) {}
  mybind_bool_t () : mybind_num_t<bool, bool> (MYSQL_TYPE_TINY) {}
  mybind_bool_t (bool *b)
    : mybind_num_t<bool, bool> (MYSQL_TYPE_TINY, b) {}
  mybind_bool_t (ptr<bool> *b)
    : mybind_num_t<bool, bool> (MYSQL_TYPE_TINY, b) {}
  operator bool () const { return isnull () ? false : n > 0; }
};

//-----------------------------------------------------------------------

class mybindable_t {
public:
  virtual ~mybindable_t () {}
#ifdef HAVE_MYSQL_BIND
  virtual void bind (MYSQL_BIND *b) = 0;
#endif
};

//-----------------------------------------------------------------------

class mybind_res_t : public mybindable_t {
public:
  mybind_res_t () {}
  virtual ~mybind_res_t () {}
  mybind_res_t (u_int64_t *i) { p = New refcounted<mybind_u64_t> (i); }
  mybind_res_t (ptr<u_int64_t> *i) { p = New refcounted<mybind_u64_t> (i); }
  mybind_res_t (str *s) { p = New refcounted<mybind_str_t> (s); }
  mybind_res_t (int64_t *i) { p = New refcounted<mybind_hyper_t> (i); }
  mybind_res_t (ptr<int64_t> *i) { p = New refcounted<mybind_hyper_t> (i); }
  mybind_res_t (int *i) { p = New refcounted<mybind_int_t> (i); }
  mybind_res_t (u_int32_t *i) { p = New refcounted<mybind_u32_t> (i); }
  mybind_res_t (ptr<u_int32_t> *i) { p = New refcounted<mybind_u32_t> (i); }
  mybind_res_t (int16_t *i) { p = New refcounted<mybind_short_t> (i); }
  mybind_res_t (ptr<int16_t> *i) { p = New refcounted<mybind_short_t> (i); }
  mybind_res_t (u_int16_t *i) { p = New refcounted<mybind_u16_t> (i); }
  mybind_res_t (ptr<u_int16_t> *i) { p = New refcounted<mybind_u16_t> (i); }
  mybind_res_t (char *i) { p = New refcounted<mybind_char_t> (i); }
  mybind_res_t (ptr<char> *i) { p = New refcounted<mybind_char_t> (i); }
  mybind_res_t (u_char *i) { p = New refcounted<mybind_u8_t> (i); }
  mybind_res_t (ptr<u_char> *i) { p = New refcounted<mybind_u8_t> (i); }
  mybind_res_t (okdatep_t d) { p = New refcounted<mybind_date_t> (d); }
  mybind_res_t (x_okdate_t *x) { p = New refcounted<mybind_date_t> (x); }
  mybind_res_t (x_okdate_time_t *x) { p = New refcounted<mybind_date_t> (x); }
  mybind_res_t (x_okdate_date_t *x) { p = New refcounted<mybind_date_t> (x); }
  mybind_res_t (bool *b) { p = New refcounted<mybind_bool_t> (b); }
  mybind_res_t (double *d) { p = New refcounted<mybind_double_t> (d); }
  template <
    typename T,
    typename dummy = typename std::enable_if<std::is_enum<T>::value, T>::type
  > mybind_res_t (T *x) :
      mybind_res_t(reinterpret_cast<typename std::underlying_type<T>::type *>(x))
  {}

  mybind_res_t (amysql_scalar_t *s) 
  { p = New refcounted<mybind_xdr_union_t> (s); }
        
  template<class C>
  mybind_res_t (union_entry<C> &u) { *this = mybind_res_t ((C *)u); }

  template<size_t n>
  mybind_res_t (rpc_bytes<n> *r) 
  { p = New refcounted<mybind_rpcstr_t<n> > (r); }

#ifdef HAVE_MYSQL_BIND
  void bind (MYSQL_BIND *bnd) { if (p) p->bind (bnd, false); }
#endif
  void assign () { if (p) p->assign (); }
  bool read_str (const char *c, unsigned long l, eft_t typ) 
  { return (p ? p->read_str (c, l, typ) : false); }

  bool is_xdr_union_type () const 
  { return p ? p->is_xdr_union_type () : false; }

  ptr<mybind_t> p;
};

//-----------------------------------------------------------------------

class mybind_param_t : public mybindable_t {
public:
  virtual ~mybind_param_t () {}
  mybind_param_t () {}
  mybind_param_t (str s) { p = New refcounted<mybind_str_t> (s); }
  mybind_param_t (double d) { p = New refcounted<mybind_double_t> (d); }
  mybind_param_t (int64_t i) { p = New refcounted<mybind_hyper_t> (i); }
  mybind_param_t (u_int64_t i) { p = New refcounted<mybind_u64_t> (i); }
  mybind_param_t (int i) { p = New refcounted<mybind_int_t> (i); }
  mybind_param_t (u_int32_t i) { p = New refcounted<mybind_u32_t> (i); }
  mybind_param_t (int16_t i) { p = New refcounted<mybind_short_t> (i); }
  mybind_param_t (u_int16_t i) { p = New refcounted<mybind_u16_t> (i); }
  mybind_param_t (char i) { p = New refcounted<mybind_char_t> (i); }
  mybind_param_t (u_char i) { p = New refcounted<mybind_u8_t> (i); }
  mybind_param_t (okdatep_t d) { p = New refcounted<mybind_date_t> (d); }
  mybind_param_t (const okdate_t &d) { p = New refcounted<mybind_date_t> (d); }
  mybind_param_t (const amysql_scalar_t &x);
  mybind_param_t (mysql_var_t v) { p = New refcounted<mybind_var_t> (v); }

  template<size_t n>
  mybind_param_t (const rpc_bytes<n> &b)
  { p = New refcounted<mybind_rpcstr_t<n> > (b); }

  mybind_param_t (const x_okdate_t &x) 
  { p = New refcounted<mybind_date_t> (x); }
  mybind_param_t (const x_okdate_date_t &x) 
  { p = New refcounted<mybind_date_t> (x); }
  mybind_param_t (const x_okdate_time_t &x) 
  { p = New refcounted<mybind_date_t> (x); }

  mybind_param_t &operator= (int64_t i) 
  { p = New refcounted<mybind_hyper_t> (i); return (*this); }
  mybind_param_t &operator= (int i) 
  { p = New refcounted<mybind_int_t> (i); return (*this); }
  mybind_param_t &operator= (double d)
  { p = New refcounted<mybind_double_t> (d); return (*this); }
  mybind_param_t &operator= (str s)
  { p = New refcounted<mybind_str_t> (s); return (*this); }
  mybind_param_t &operator= (u_int32_t i)
  { p = New refcounted<mybind_u32_t> (i); return (*this); }
  mybind_param_t &operator= (int16_t i)
  { p = New refcounted<mybind_short_t> (i); return (*this); }
  mybind_param_t &operator= (char i)
  { p = New refcounted<mybind_char_t> (i); return (*this); }
  mybind_param_t &operator= (u_int16_t i)
  { p = New refcounted<mybind_u16_t> (i); return (*this); }
  mybind_param_t &operator= (u_char i)
  { p = New refcounted<mybind_u8_t> (i); return (*this); }
  mybind_param_t &operator= (okdatep_t d)
  { p = New refcounted<mybind_date_t> (d); return (*this); }
  mybind_param_t &operator= (const okdate_t &d)
  { p = New refcounted<mybind_date_t> (d); return (*this); }
  mybind_param_t &operator= (const x_okdate_t &x)
  { p = New refcounted<mybind_date_t> (x); return (*this); }
  mybind_param_t &operator= (const x_okdate_time_t &x)
  { p = New refcounted<mybind_date_t> (x); return (*this); }
  mybind_param_t &operator= (const x_okdate_date_t &x)
  { p = New refcounted<mybind_date_t> (x); return (*this); }
  mybind_param_t &operator= (const mysql_var_t &v)
  { p = New refcounted<mybind_var_t> (v); return (*this); }
  
  template<size_t n> 
  mybind_param_t &operator= (const rpc_bytes<n> &b)
  { 
    str s (b.base (), b.len ());
    p = New refcounted<mybind_str_t> (s); return (*this);
  }
  
#ifdef HAVE_MYSQL_BIND
  void bind (MYSQL_BIND *bnd) { p->bind (bnd, true); }
#endif
  void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l) 
  { p->to_qry (m, b, s, l); }
  str to_str () const { return p->to_str (); }
  ptr<mybind_t> p;
};

//=======================================================================

class global_gmt_offset_t {
public:
  global_gmt_offset_t () : _when_updated (0), _val (0) {}
  bool get (long *val, time_t freshness = 0);
  long get () { return _val; }
  void set (long v);
private:
  time_t _when_updated;
  long _val;
};

//=======================================================================

extern global_gmt_offset_t global_gmt_offset;

//=======================================================================

#ifdef HAVE_MYSQL_BIND
template<class C> void 
mbdxat_t<C>::assign (const MYSQL_TIME &tm, okdatep_t d)
{ mysql_to_xdr (tm, pntr); d->set (*pntr, global_gmt_offset.get ()); }
#endif

//=======================================================================

u_int next2pow (u_int i);

//=======================================================================

#endif /* HAVE_MYSQL */
