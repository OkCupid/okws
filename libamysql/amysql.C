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

#include "async.h"
#include "amysql.h"
#include "mystmt.h"

bool
mysql_t::connect (const dbparam_t &p)
{
  return connect (p._database, p._user, p._host, p._pw,
		  p._port, p._flags);
}

bool 
mysql_t::connect (const str &db, const str &u, const str &h,
		  const str &pw, u_int prt, u_long fl)
{
  bool ret = true;
#if defined(MYSQL_VERSION_ID) && (MYSQL_VERSION_ID >= 50000)
   my_bool b = 1;
   if (mysql_options (&mysql, MYSQL_OPT_RECONNECT, (const char *)&b) != 0) {
      fprintf (stderr, "cannot set reconnect on MySQL object\n");
   }
#endif /* MYSQL_VERSION_ID */

  if (!mysql_real_connect (&mysql, h, u, pw, db, prt, NULL, fl)) {
    err = strbuf ("connection error: ") << mysql_error (&mysql);
    ret = false;
  }
  return ret;
}

sth_t
mysql_t::prepare (const str &q, u_int l_opts, tz_corrector_t *tzc)
{
  if (l_opts & AMYSQL_DEFAULT)
    l_opts = opts;
  sth_t *rp = cache[q];
  if (rp) return (*rp);

  sth_t r = NULL;
  if (l_opts & AMYSQL_PREPARED) {
#if defined(HAVE_MYSQL_BINDFUNCS) && defined(HAVE_MYSQL_BIND)
    MYSQL_STMT *s = mysql_stmt_init (&mysql);
    if (!s) {
      err = strbuf ("MySQL ran out of memory on statment init: ")
	<< mysql_error (&mysql);
      return NULL;
    }

    if (mysql_stmt_prepare (s, q, q.len ())) {
      err = strbuf ("could not prepare query (") 
	<< q << "): " << mysql_error (&mysql);
      return NULL;
    }
    r = sth_prepared_t::alloc (s, q, l_opts, tzc);
#endif // HAVE_MYSQL_BINDFUNCS && HAVE_MYSQL_BIND
  } else {
    ptr<sth_parsed_t> r2 = sth_parsed_t::alloc (&mysql, q, l_opts, tzc);
    if (!r2->parse ())
      return NULL;
    r = r2;
  }
  if (!(l_opts & AMYSQL_NOCACHE))
    cache.insert (q, r);
  return r;
}

sth_t
amysql_thread_t::prepare (const str &q, u_int o)
{
  if (readied) {
    TWARN ("security precaution: cannot prepare queries "
	   "after servicing requests\n");
    return NULL;
  }
	   
  sth_t r = mysql.prepare (q, o, _tzc);
  if (!r) 
    TWARN ("prepare query failed: " << q);
  return r;
}

amysql_thread_t::amysql_thread_t (mtd_thread_arg_t *a, u_int o)
  : mtd_thread_t (a), mysql (o),
    _tzc ( (o & AMYSQL_NOTZCORRECT) ? NULL : New tz_corrector_t (*this)) 
{}

#ifdef HAVE_MYSQL_BIND
void
mybind_str_t::bind (MYSQL_BIND *bnd, bool param)
{
  bnd->buffer_type = ft;
  bnd->buffer = buf;
  bnd->buffer_length = size;
  bnd->length = param ? &bnd->buffer_length : &len;
  bnd->is_null = param ? 0 : &nullfl;
}
#endif

void
mybind_str_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  u_int len2 = (size << 1) + 1;
  if (*l < len2 || !*s) {
    *l = next2pow (len2);
    if (*s)
      delete [] *s;
    *s = New char[*l];
  }
  u_int rlen = mysql_real_escape_string (m, *s + 1, buf, size);
  (*s)[0] = (*s)[rlen + 1] = '\'';
  b->buf (*s, rlen + 2);
}

str mybind_str_t::to_str () const { return str (buf, size); }

void
mybind_sex_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  buf[1] = sex_to_char (sx);
  b->buf (buf, 3);
}

str 
mybind_sex_t::to_str () const 
{
  char b[2];
  b[1] = 0;
  b[0] = sex_to_char (sx);
  return str (b, 2);
}


u_int
next2pow (u_int i)
{
  u_int log = 0;
  while (i) {
    i = i >> 1;
    log++;
  }
  return (max (1 << (log + 1), BIND_BUF_MIN)); 
}

static eft_t 
to_mysql_typ (const okdate_t &d)
{
  if (!d.err) {
    if (d.dt_tm & ( OK_DATE | OK_TIME)) {
      return MYSQL_TYPE_DATETIME;
    } else if (d.dt_tm & OK_DATE) {
      return MYSQL_TYPE_DATE;
    } else if (d.dt_tm & OK_TIME) {
      return MYSQL_TYPE_TIME;
    }
  }
  return MYSQL_TYPE_DATETIME;
}

static eft_t
to_mysql_typ (const x_okdate_t &x) 
{
  if (x.date.on && x.time.on) {
    return MYSQL_TYPE_DATETIME;
  } else if (x.date.on) {
    return MYSQL_TYPE_DATE;
  } else {
    return MYSQL_TYPE_TIME;
  }
}

mybind_date_t::mybind_date_t (okdatep_t d) 
  : mybind_t (to_mysql_typ (*d)), datep (d), pntr (NULL), parsed (false),
    palloced (false) {}

mybind_date_t::mybind_date_t (const x_okdate_t &d)
  : mybind_t (to_mysql_typ (d)), datep (okdate_t::alloc (d)), pntr (NULL),
    parsed (false), palloced (false) {}

void
mybind_date_t::to_qry (MYSQL *m, strbuf *b, char **s, u_int *l)
{
  datep->to_strbuf (b, true, global_gmt_offset.get ());
}

str
mybind_date_t::to_str () const 
{
  strbuf b;
  datep->to_strbuf (&b, true, global_gmt_offset.get ());
  return b;
}


#ifdef HAVE_MYSQL_BIND
void
mybind_date_t::bind (MYSQL_BIND *bnd, bool param)
{
  bnd->buffer_type = ft;
  bnd->buffer = (char *)&tm;
  bnd->buffer_length = sizeof (tm);
  bnd->length = param ? &bnd->buffer_length : &len;
  bnd->is_null = param ? 0 : &nullfl;
}

void
mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_t *x)
{
  x->date.set_on (true);
  x->time.set_on (true);
  mysql_to_xdr (tm, x->date.date);
  mysql_to_xdr (tm, x->time.time);
}

void
mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_time_t *x)
{
  x->sec = tm.second;
  x->min = tm.minute;
  x->hour = tm.hour;
}

void
mysql_to_xdr (const MYSQL_TIME &tm, x_okdate_date_t*x)
{
  x->mday = tm.day;
  x->mon = tm.month;
  x->year = tm.year;
}

#endif

bool
mybind_date_t::read_str (const char *c, unsigned long)
{
  parsed = true;
  datep->set (c, global_gmt_offset.get ());
  if (pntr)
    pntr->parsed_assign (datep);
  return (!datep->err);
}

bool
amysql_thread_t::init_phase0 ()
{
  return (!_tzc || _tzc->prepare ());
}
