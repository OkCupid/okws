/* $Id: pt1d.C 1007 2005-09-11 21:45:33Z max $ */

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
#include "tst2_prot.h"
#include "pslave.h"

#include "okwsconf.h"
#ifdef HAVE_MYSQL
#include "amysql.h"
#include "mystmt.h"
#include "web.h"

TYPE2STRUCT( , adb_status_t);

class tst2_srv_t : public amysql_thread_t {
public:
  tst2_srv_t (mtd_thread_arg_t *a, int meth) 
   : amysql_thread_t (a, meth), err (false) {}
  bool init ();
  void dispatch (svccb *sbp);
  static mtd_thread_t *alloc (int meth, mtd_thread_arg_t *arg)
  { return New tst2_srv_t (arg, meth); }
protected:
  void get (svccb *sbp);
  void put (svccb *sbp);
private:
  sth_t _q_get, _q_put;
  bool err;
};

//static void apply_test (str s, u_int i, pt1_srv_t *t) { t->pairset (s, i); }

bool
tst2_srv_t::init ()
{
  bool rc = true;
  if (!mysql.connect ("okws_db_tst2", "okws", "localhost", "abc123")) {
    TWARN (mysql.error ());
    rc = false;
  } else if (!(_q_get = PREP("SELECT id,d,i FROM tst2 WHERE s = ?")) ||
	     !(_q_put = PREP("INSERT INTO tst2(s,d,i) VALUES(?,?,?)"))) {
    rc = false;
  }
  return rc;

}

void
tst2_srv_t::dispatch (svccb *sbp)
{
  //int id = getid ();
  u_int p = sbp->proc ();
  err = false;
  switch (p) {
  case TST2_PUT:
    put (sbp);
    break;
  case TST2_GET:
    get (sbp);
    break;
  default:
    reject ();
    break;
  }
}

void
tst2_srv_t::get (svccb *b)
{
  const tst2_get_arg_t *arg  = b->Xtmpl getarg<tst2_get_arg_t> ();
  adb_status_t rc;
  ptr<tst2_get_res_t> res = New refcounted<tst2_get_res_t> (ADB_OK);
  
  if (!_q_get->execute (*arg)) {
    rc = ADB_EXECUTE_ERROR;
  } else {
    rc = _q_get->fetch (&res->dat->pk, &res->dat->d, &res->dat->i);
  }

  if (rc != ADB_OK) {
    res->set_status (rc);
    str es;
    if ((es = _q_get->error ())) 
      TWARN(es);
  }
  reply (res);
}

void
tst2_srv_t::put (svccb *b)
{
  const tst2_put_arg_t *arg = b->Xtmpl getarg<tst2_put_arg_t> ();
  ptr<adb_status_t> res = New refcounted<adb_status_t> (ADB_OK);

  if (!_q_put->execute (arg->key, arg->data.d, arg->data.i)) {
    *res = ADB_EXECUTE_ERROR;
    str es;
    if ((es = _q_put->error ())) 
      TWARN("put error: " << es);
  }
  reply (res);

}

static void
usage ()
{
  warnx << "usage: " << progname << " [-p] [-t <n-threads>] [-q <qsiz>]\n";
  exit (1);
}

static void
start_server (int argc, char *argv[])
{
  int tcnt = 10;
  int maxq = 1000;
  int mysql_sth_method = 0;

  int ch;

  while ((ch = getopt (argc, argv, "t:q:ph")) != -1)
    switch (ch) {
    case 't':
      if (!convertint (optarg, &tcnt)) usage ();
      break;
    case 'q':
      if (!convertint (optarg, &maxq)) usage ();
      break;
    case 'p':
      mysql_sth_method = AMYSQL_PREPARED;
      break;
    default:
      usage ();
    }

  argc -= optind;
  argv += optind;

  if (argc != 0)
    usage ();
  
  ssrv_t *s = New ssrv_t (wrap (&tst2_srv_t::alloc, mysql_sth_method), 
			  tst2_prog_1, MTD_PTH, tcnt, maxq);
  
  // turn off all warning messages
  s->mtd->set_quiet (true);
  
  if (!pub_server (wrap (s, &ssrv_t::accept), TST2_PORT))
    fatal << "Cannot bind to port " << TST2_PORT << "\n";
}


#endif  /* HAVE_MYSQL */

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
#ifdef HAVE_MYSQL
  start_server (argc, argv);
  amain ();
#else 
  warn << "No MySQL support; test failed!\n";
  exit (1);
#endif
  return 0;
}
