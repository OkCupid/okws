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
#include "pt1_prot.h"
#include "pslave.h"

#include "okwsconf.h"
#ifdef HAVE_MYSQL
#include "amysql.h"
#include "mystmt.h"
#include "web.h"

//typedef callback<void, ptr<t1_user_t> >::ref usercb_t;
class pt1_srv_t : public amysql_thread_t {
public:
 pt1_srv_t (mtd_thread_arg_t *a) 
   : amysql_thread_t (a, AMYSQL_PREPARED), err (false) {}
  void dispatch (svccb *sbp);
  bool init ();
  static mtd_thread_t *alloc (mtd_thread_arg_t *arg) 
  { return New pt1_srv_t (arg); }
  void pairset (str s, u_int i) { _s = s; _i = i; }
protected:
  void insert (svccb *sbp);
  void lookup (svccb *sbp);
  void times_tab (svccb *sbp);
  adb_status_t lookup (int id, ptr<mystmt_t> sth, mybind_param_t p);
private:
  sth_t q_em, q_nm, q_id, ins;
  sth_t qry, tt;
  bool err;
  
  // test a new feature
  str _s;
  int _i;
};

//static void apply_test (str s, u_int i, pt1_srv_t *t) { t->pairset (s, i); }

bool
pt1_srv_t::init ()
{
  if (!mysql.connect ("pt1", "root", "localhost")) {
    TWARN (mysql.error ());
    return false;
  }
  if(!(qry = PREP("SELECT * FROM sha1_tab WHERE x = ?")))
     return false;

	/*
  if (!(tt = PREP("SELECT * FROM times_tab LIMIT 60")))
    return false;
	*/
  return true;
}

void
pt1_srv_t::dispatch (svccb *sbp)
{
  //int id = getid ();
  u_int p = sbp->proc ();
  err = false;
  switch (p) {
  case PT1_NULL:
    replynull ();
    break;
  case PT1_QUERY:
    lookup (sbp);
    break;
  case PT1_TIMES_TAB:
    times_tab (sbp);
    break;
  default:
    reject ();
  }
}

void
pt1_srv_t::times_tab (svccb *b)
{
  ptr<pt1_times_tab_res_t> u = 
    New refcounted<pt1_times_tab_res_t> (ADB_EXECUTE_ERROR);
  str err;
  if (tt->execute ()) {
    u->set_status (ADB_OK);
    for (int i = 0; i < TT_ROWS; i++) {
      u_int32_t *b = (*u->tab)[i].base ();
      tt->fetch (b, b+1, b+2, b+3, b+4, b+5, b+6, b+7, b+8, b+9 );
    }
  }
  if ((err = tt->error ()))
    TWARN(err);
  reply (u);
}

void
pt1_srv_t::lookup (svccb *b)
{
  ptr<pt1_query_res_t> u = New refcounted<pt1_query_res_t> (ADB_ERROR);
  int *arg = b->Xtmpl getarg<int> ();
  str err;
  adb_status_t rc = ADB_OK;
  u->set_status (ADB_OK);
  if(!qry->execute(*arg))
    rc =  ADB_EXECUTE_ERROR;
  else 
    qry->fetch (&u->out->id, &u->out->sha1);
  u->set_status (rc);
  if ((err = qry->error ())) 
    TWARN(err);
  reply (u);
}

#endif  /* HAVE_MYSQL */

int
main (int argc, char *argv[])
{
#ifdef HAVE_MYSQL
  int cnt = 50;
  int maxq = 10000;
  int tmp;
  if (argc > 1 && convertint (argv[1], &tmp))
    cnt = tmp;
  ssrv_t *s = New ssrv_t (wrap (&pt1_srv_t::alloc), pt1_prog_1, MTD_PTH, 
			  cnt, maxq);

  // turn off all warning messages
  s->mtd->set_quiet (true);

  if (!pub_server (wrap (s, &ssrv_t::accept), PT1_PORT))
    fatal << "Cannot bind to port " << PT1_PORT << "\n";
  amain ();
#endif
#ifndef HAVE_MYSQL
  exit (1);
#endif
}
