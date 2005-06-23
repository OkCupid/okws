// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003-4 by Maxwell Krohn (max@okcupid.com)
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

#include "pslave.h"
#include "okwsconf.h"
#include "amysql.h"
#include "mystmt.h"
#include "web.h"
#include "sha_prot.h"

class shad_dbprox_t : public amysql_thread_t {
public:
  shad_dbprox_t (mtd_thread_arg_t *a) 
    : amysql_thread_t (a, AMYSQL_PREPARED), err (false) {}
  void dispatch (svccb *sbp);
  bool init ();
  static mtd_thread_t *alloc (mtd_thread_arg_t *arg) 
  { return New shad_dbprox_t (arg); }
  void query (svccb *b);
protected:
private:
  sth_t qry;
  bool err;
};

bool
shad_dbprox_t::init ()
{
  if (!mysql.connect ("oktest", "root", "localhost")) {
    TWARN (mysql.error ());
    return false;
  }
  if (!(qry = PREP ("SELECT y FROM sha1 WHERE x = ?")))
    return false;
  return true;
}

void
shad_dbprox_t::dispatch (svccb *sbp)
{
  u_int p = sbp->proc ();

  switch (p) {
  case SHA_QUERY2:
  case SHA_QUERY:
    query (sbp);
    break;
  default:
    reject ();
  }
}

void
shad_dbprox_t::query (svccb *b)
{
  ptr<sha_query_res_t> r = New refcounted<sha_query_res_t> (ADB_OK);
  sha_query_arg_t *arg = b->Xtmpl getarg<sha_query_arg_t> ();
  adb_status_t err_rc = ADB_EXECUTE_ERROR;
  if (!qry->execute (*arg) || (err_rc = qry->fetch (r->res)) != ADB_OK) 
    r->set_status (err_rc);
  reply (r);
}

int
main (int argc, char *argv[])
{
  ssrv_t *s = New ssrv_t (wrap (&shad_dbprox_t::alloc), 
			  sha_prog_1, MTD_PTH, 1, MTD_MAXQ,
			  New sha_prog_1_txa_t ());
  if (!pub_server (wrap (s, &ssrv_t::accept), SHAD_PORT))
    fatal << "Cannot bind to port " << SHAD_PORT << "\n";
  amain ();
}
