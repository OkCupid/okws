
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAMYSQL_AMYSQL_H
#define _LIBAMYSQL_AMYSQL_H

#include "okwsconf.h"
#include "async.h"

#include "mysql.h"
#include "str.h"
#include "amt.h"
#include "amysql_prot.h"
#include "web_prot.h"
#include "web.h"
#include "qhash.h"
#include "parseopt.h"
#include "pub.h"

TYPE2STRUCT(, unsigned long long);
TYPE2STRUCT(, long long);

#define BIND_BUF_MIN 256

u_int next2pow (u_int i);

#define AMYSQL_NOCACHE  (1 << 0)  /* cache prepared querieds in hashtab */
#define AMYSQL_PREPARED (1 << 1)  /* use prepared statement handles */
#define AMYSQL_DEFAULT  (1 << 2)  /* use global object defaults */
#define AMYSQL_USERES   (1 << 3)  /* call use res as opposed to store res */

#define MYSQL_DATE_STRLEN 20     // strlen ("YYYY-MM-DD HH:MM:SS")

typedef enum {
  FETCH_OK = 0,
  FETCH_ERROR = 1, 
  FETCH_NO_DATA = 2,
  FETCH_BIND_ERROR = 3
} fetch_status_t;

typedef enum enum_field_types eft_t;

class mybind_t {
public:
  mybind_t (eft_t t) : ft (t), nullfl (0) {}
  virtual ~mybind_t () {}
  virtual void bind (MYSQL_BIND *bind, bool param) = 0;
  virtual void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l) = 0;
  virtual bool isnull () const { return nullfl; }
  virtual void assign () {}
  virtual bool read_str (const char *c, unsigned long l) = 0;
protected:
  eft_t ft;
  my_bool nullfl;
};

void mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_t *x);
void mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_date_t *x);
void mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_time_t *x);

class mybind_date_xassign_t {
public:
  mybind_date_xassign_t () {}
  virtual void assign (const MYSQL_TIME &tm, okdatep_t d) = 0;
  virtual void assign (okdatep_t d) = 0;
  virtual ~mybind_date_xassign_t () {}
};

template<class C> // MyBind Date Xassign Template 
class mbdxat_t : public mybind_date_xassign_t {
public:
  mbdxat_t (C *p) : mybind_date_xassign_t (), pntr (p) {}
  void assign (const MYSQL_TIME &tm, okdatep_t d) 
  { mysql_to_xdr (tm, pntr); d->set (*pntr); }
  void assign (okdatep_t d) { d->to_xdr (pntr); }
private:
  C *pntr;
};

class mybind_date_t : public mybind_t {
public:
  mybind_date_t (okdatep_t d);

  mybind_date_t (const x_okdate_t &d);
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

  void assign () 
  {
    if (pntr) {
      if (parsed) pntr->assign (datep);
      else pntr->assign (tm, datep);
    }
  }

  operator okdatep_t () const 
  { return isnull () ? static_cast<okdatep_t> (NULL) : datep; }
  bool read_str (const char *c, unsigned long l);
  void bind (MYSQL_BIND *bind, bool param);
private:
  MYSQL_TIME tm;
  okdatep_t datep;
  mybind_date_xassign_t *pntr;
  u_long len;
  bool parsed;
  bool palloced;
};

class mybind_str_t : public mybind_t {
public:
  mybind_str_t (const str &s, eft_t t = MYSQL_TYPE_STRING, bool hld = false) :
    mybind_t (t), hold (hld ? s : sNULL), size (s.len ()), 
    buf (const_cast<char *> (s.cstr ())), balloced (false),
    pntr (NULL) {}

  mybind_str_t (sex_t s) 
    : mybind_t (MYSQL_TYPE_STRING), hold (sex_to_str (s)), size (hold.len ()),
      buf (const_cast<char *> (hold.cstr ())), balloced (false),
      pntr (NULL) {}
  
  mybind_str_t (u_int s = 1024, eft_t t = MYSQL_TYPE_STRING) : 
    mybind_t (t), hold (NULL), size (s), buf (New char[s]), balloced (true),
    pntr (NULL) {}
  
  mybind_str_t (str *p, u_int s = 1024, eft_t t = MYSQL_TYPE_STRING) :
    mybind_t (t), hold (NULL), size (s), buf (New char[s]), balloced (true),
    pntr (p) {}
  
  ~mybind_str_t () { if (balloced) delete [] buf; }
  void bind (MYSQL_BIND *bnd, bool param);
  virtual void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l);
  virtual void assign () { *pntr = str (buf, len); }
  operator str () const { return isnull () ? sNULL : str (buf, len); }
  bool read_str (const char *c, unsigned long l) 
  { *pntr = str (c, l); return true; }
protected:
  str hold;
  u_int size;
  char *buf;
  u_long len;
  bool balloced;
  str *pntr;
};

template<size_t n>
class mybind_rpcstr_t :  public mybind_t {
public:
  mybind_rpcstr_t (rpc_bytes<n> *rr) 
    : mybind_t (MYSQL_TYPE_STRING), pntr (rr), mys (&s) {}
  mybind_rpcstr_t (const rpc_bytes<n> &in)
    : mybind_t (MYSQL_TYPE_STRING), 
      mys (str (in.base (), in.size ()), MYSQL_TYPE_STRING, true) {}
  void bind (MYSQL_BIND *bind, bool param) { mys.bind (bind, param); }
  void to_qry (MYSQL *m, strbuf *b, char **ss, u_int *l)
  { mys.to_qry (m, b, ss, l); }
  void assign () { mys.assign (); *pntr = s; }
  bool read_str (const char *c, unsigned long l) 
  {
    if (!mys.read_str (c, l)) return false;
    *pntr = s;
    return true;
  }
protected:
  rpc_bytes<n> *pntr;
  str s;
  mybind_str_t mys;
};

class mybind_sex_t : public mybind_str_t {
public:
  mybind_sex_t (sex_t *p) : mybind_str_t (), pntr (p), sx (NOSEX) {}
  mybind_sex_t (sex_t s) : mybind_str_t (s), sx (s) 
  { sprintf (buf, "\'%c\'", sex_to_char (sx)); }
  virtual void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l);
  virtual void assign ()
  {
    *pntr = size ? str_to_sex (str (buf, size)) : NOSEX;
  }
  virtual bool read_str (const char *c, unsigned long l)
  {
    return ((*pntr = c ? char_to_sex (*c) : NOSEX) != NOSEX);
  }
private:
  char buf[4];
  sex_t *pntr;
  sex_t sx;
};

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
  virtual void bind (MYSQL_BIND *bnd, bool param)
  {
    mysql_n = n;
    bnd->buffer_type = ft;
    bnd->buffer = (char *)&mysql_n;
    bnd->is_null = param ? 0 : &nullfl;
    bnd->length = param ? 0 : &mysql_blows;
    bnd->buffer_length = sizeof (mysql_n);
  }
  
  inline void pntr_assign ()
  {
    if (pntr)
      *pntr = n;
    if (ppntr) {
      if (nullfl) *ppntr = NULL;
      else *ppntr = New refcounted<T> (n);
    }
  }

  virtual void assign ()
  {
    if (nullfl)
      mysql_n = 0;
    n = mysql_n;
    pntr_assign ();
  }

  virtual bool read_str (const char *c, unsigned long)
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
  virtual bool read_str (const char *c, unsigned long)
  {
    if (!c || ( (n = strtoull (c, NULL, 10)) == 0 && errno == EINVAL)) {
      n = 0;
      nullfl = true;
    }
    pntr_assign ();
    return true;
  }
};

class mybind_int_t : public mybind_num_t<int, long> {
public:
  mybind_int_t (int i) : mybind_num_t<int, long> (MYSQL_TYPE_LONG, i) {}
  mybind_int_t (int *i) : mybind_num_t<int, long> (MYSQL_TYPE_LONG, i) {}
  mybind_int_t () : mybind_num_t<int, long> (MYSQL_TYPE_LONG) {}
  mybind_int_t (ptr<int> *i) : mybind_num_t<int, long> (MYSQL_TYPE_LONG, i) {}
};

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

class mybindable_t {
public:
  virtual void bind (MYSQL_BIND *b) = 0;
};

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
  mybind_res_t (sex_t *s) { p = New refcounted<mybind_sex_t> (s); }
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
        
  template<class C>
  mybind_res_t (union_entry<C> &u) { *this = mybind_res_t ((C *)u); }

  template<size_t n>
  mybind_res_t (rpc_bytes<n> *r) 
  { p = New refcounted<mybind_rpcstr_t<n> > (r); }

  void bind (MYSQL_BIND *bnd) { if (p) p->bind (bnd, false); }
  void assign () { if (p) p->assign (); }
  bool read_str (const char *c, unsigned long l) 
  { return (p ? p->read_str (c, l) : false); }
  ptr<mybind_t> p;
};

class mybind_param_t : public mybindable_t {
public:
  virtual ~mybind_param_t () {}
  mybind_param_t () {}
  mybind_param_t (str s) { p = New refcounted<mybind_str_t> (s); }
  mybind_param_t (int64_t i) { p = New refcounted<mybind_hyper_t> (i); }
  mybind_param_t (u_int64_t i) { p = New refcounted<mybind_u64_t> (i); }
  mybind_param_t (int i) { p = New refcounted<mybind_int_t> (i); }
  mybind_param_t (u_int32_t i) { p = New refcounted<mybind_u32_t> (i); }
  mybind_param_t (int16_t i) { p = New refcounted<mybind_short_t> (i); }
  mybind_param_t (u_int16_t i) { p = New refcounted<mybind_u16_t> (i); }
  mybind_param_t (char i) { p = New refcounted<mybind_char_t> (i); }
  mybind_param_t (u_char i) { p = New refcounted<mybind_u8_t> (i); }
  mybind_param_t (okdatep_t d) { p = New refcounted<mybind_date_t> (d); }
  mybind_param_t (sex_t s) { p = New refcounted<mybind_sex_t> (s); }

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
  mybind_param_t &operator= (sex_t s)
  { p = New refcounted<mybind_sex_t> (s); return (*this); }
  mybind_param_t &operator= (const x_okdate_t &x)
  { p = New refcounted<mybind_date_t> (x); return (*this); }
  mybind_param_t &operator= (const x_okdate_time_t &x)
  { p = New refcounted<mybind_date_t> (x); return (*this); }
  mybind_param_t &operator= (const x_okdate_date_t &x)
  { p = New refcounted<mybind_date_t> (x); return (*this); }
  
  template<size_t n> 
  mybind_param_t &operator= (const rpc_bytes<n> &b)
  { 
    str s (b.base (), b.len ());
    p = New refcounted<mybind_str_t> (s); return (*this);
  }
  
  void bind (MYSQL_BIND *bnd) { p->bind (bnd, true); }
  void to_qry (MYSQL *m, strbuf *b, char **s, u_int *l) 
  { p->to_qry (m, b, s, l); }
  ptr<mybind_t> p;
};

class mystmt_t;
class mysql_t {
public:
  mysql_t (u_int o = 0) : opts (o) { mysql_init (&mysql); }
  ~mysql_t () { mysql_close (&mysql); }
  ptr<mysql_t> mysql_t::alloc () { return New refcounted<mysql_t> (); }
  bool connect (const str &d, const str &h = NULL, const str &u = NULL, 
		const str &pw = NULL, u_int prt = 0, u_long fl = 0);
  str error () const { return err; }
  ptr<mystmt_t> prepare (const str &q, u_int opts = AMYSQL_DEFAULT);
  u_int64_t insert_id () { return mysql_insert_id (&mysql); }
  u_int64_t affected_rows() { return mysql_affected_rows(&mysql);}
  u_int64_t warning_count() { return mysql_warning_count(&mysql);}
  
  u_int opts;
  str err;
  MYSQL mysql;
  qhash<str, ptr<mystmt_t> > cache;
};

class amysql_thread_t : public mtd_thread_t {
public:
  amysql_thread_t (mtd_thread_arg_t *a, u_int o = 0) 
    : mtd_thread_t (a), mysql (o) {}
  virtual ~amysql_thread_t () {}
  ptr<mystmt_t> prepare (const str &q, u_int opts = AMYSQL_DEFAULT);
protected:
  mysql_t mysql;
};

#define PREP(x) prepare (strbuf () << x)



#endif /* _LIBAMYSQL_AMYSQL_H */
