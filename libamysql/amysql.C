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
#include "pubutil.h"

//-----------------------------------------------------------------------

bool
mysql_t::connect (const dbparam_t &p)
{
  return connect (p._database, p._user, p._host, p._pw,
		  p._port, p._flags);
}

//-----------------------------------------------------------------------

bool 
mysql_t::connect (const str &db, const str &u, const str &h,
		  const str &pw, u_int prt, u_long fl)
{
  bool ret = true;

  // Use the default mysql port if none was provided.
  if (prt == 0) { prt = 3306; }

#if defined(MYSQL_VERSION_ID) && (MYSQL_VERSION_ID >= 50000)
   my_bool b = 1;
   if (mysql_options (&mysql, MYSQL_OPT_RECONNECT, (const char *)&b) != 0) {
      fprintf (stderr, "cannot set reconnect on MySQL object\n");
   }
#endif /* MYSQL_VERSION_ID */

   GIANT_UNLOCK();
   MYSQL *rc = mysql_real_connect (&mysql, h.cstr(), u.cstr(), pw.cstr(),
                                   db.cstr(), prt, NULL, fl);
   GIANT_LOCK();

   if (!rc) {
    err = strbuf ("connection error: ") << mysql_error (&mysql);
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

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
    GIANT_UNLOCK();
    MYSQL_STMT *s = mysql_stmt_init (&mysql);
    GIANT_LOCK();
    if (!s) {
      err = strbuf ("MySQL ran out of memory on statment init: ")
	<< mysql_error (&mysql);
      errcode = ADB_BAD_PREPARE;
      return NULL;
    }

    GIANT_UNLOCK();
    int rc = mysql_stmt_prepare (s, q.cstr(), q.len ());
    GIANT_LOCK();

    if (rc) {
      err = strbuf ("could not prepare query (") 
	<< q << "): " << mysql_error (&mysql);
      errcode = ADB_BAD_QUERY;
      return NULL;
    }
    r = sth_prepared_t::alloc (s, q, l_opts, tzc);
#endif // HAVE_MYSQL_BINDFUNCS && HAVE_MYSQL_BIND
  } else {
    ptr<sth_parsed_t> r2 = sth_parsed_t::alloc (&mysql, q, l_opts, tzc);
    if (!r2->parse ()) {
      err = "failed to parse query";
      errcode = ADB_BAD_QUERY;
      return NULL;
    }
    r = r2;
  }
  if (!(l_opts & AMYSQL_NOCACHE))
    cache.insert (q, r);
  return r;
}

//-----------------------------------------------------------------------

sth_t
amysql_thread_guts_t::prepare (const str &q, u_int o, mysql_t *m)
{
  sth_t r;
  if (!m) { m = &mysql; }
  if (is_readied () && is_safe ()) {
    _err = "security precaution: cannot prepare queries "
      "after servicing requests\n";
    TWARN(_err);
    _errcode = ADB_SECURITY_FAILURE;
  } else if (!(r = m->prepare (q, o, _tzc))) {
    _err = m->error ();
    _errcode = m->error_code ();
    TWARN ("prepare query failed: " << q);
  }
  return r;
}

//-----------------------------------------------------------------------

bool
amysql_thread_guts_t::init_phase0 ()
{
  return (!_tzc || _tzc->prepare ());
}

//-----------------------------------------------------------------------

amysql_thread_guts_t::amysql_thread_guts_t (u_int o)
  : mysql (o),
    _tzc ( (o & AMYSQL_NOTZCORRECT) ? NULL : New tz_corrector_t (this)),
    _errcode (ADB_OK),
    _safe (true) {}

//-----------------------------------------------------------------------
