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
#include "json_rpc.h"

class tst2_srv_t : public amysql_thread2_t {
public:
  tst2_srv_t (mtd_thread_arg_t *a, int meth) 
   : amysql_thread2_t (a, meth), err (false) {}
  bool init ();
  void dispatch (ptr<amt::req_t> b);
  static mtd_thread_t *alloc (int meth, mtd_thread_arg_t *arg)
  { return New tst2_srv_t (arg, meth); }
protected:
  void get (ptr<amt::req_t> b);
  void mget (ptr<amt::req_t> b);
  void put (ptr<amt::req_t> b);
  void foo_reflect (ptr<amt::req_t> b);
  void negate (ptr<amt::req_t> b);
  void sum (ptr<amt::req_t> b);
private:
  sth_t _q_get, _q_put, _q_mget;
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
  } else if (!(_q_get = PREP("SELECT id,d,i,d2 FROM tst2 WHERE s = ?")) ||
	     !(_q_put = PREP("INSERT INTO tst2(s,d,i,d2) VALUES(?,?,?,?)")) ||
	     !(_q_mget = PREP("SELECT SLEEP(?/1000),id,d,i,d2 "
			      "FROM tst2 ORDER BY RAND() "
			      "LIMIT ?"))) {
    rc = false;
  }
  return rc;

}

void
tst2_srv_t::dispatch (ptr<amt::req_t> sbp)
{
  //int id = getid ();
  u_int p = sbp->proc ();
  err = false;
  switch (p) {
  case TST2_NULL:
    sbp->replynull ();
    break;
  case TST2_PUT:
    put (sbp);
    break;
  case TST2_GET:
    get (sbp);
    break;
  case TST2_FOO_REFLECT:
    foo_reflect (sbp);
    break;
  case TST2_NEGATE:
    negate (sbp);
    break;
  case TST2_SUM:
    sum (sbp);
    break;
  case TST2_MGET:
    mget (sbp);
    break;
  default:
    sbp->reject ();
    break;
  }
}

//-----------------------------------------------------------------------

void
tst2_srv_t::foo_reflect (ptr<amt::req_t> b)
{
  rpc::tst2_prog_1::tst2_foo_reflect_srv_t<amt::req_t> srv (b);
  const foo_t *arg = srv.getarg ();
  ptr<foo_t> res = srv.alloc_res (*arg);
  srv.reply (res);
}

//-----------------------------------------------------------------------

void
tst2_srv_t::sum (ptr<amt::req_t> b)
{
  rpc::tst2_prog_1::tst2_sum_srv_t<amt::req_t> srv (b);
  const u64_vec_t *arg = srv.getarg ();
  
  u_int64_t res = 0;
  for (size_t i = 0; i < arg->size (); i++) {
    res += (*arg)[i];
  }
  srv.reply (srv.alloc_res (res));
}

//-----------------------------------------------------------------------

void 
tst2_srv_t::negate (ptr<amt::req_t> b)
{
  rpc::tst2_prog_1::tst2_negate_srv_t<amt::req_t> srv (b);
  const bool *arg = srv.getarg ();
  ptr<bool> res = srv.alloc_res ();
  *res = !*arg;
  srv.reply (res);
}

//-----------------------------------------------------------------------

void
tst2_srv_t::mget (ptr<amt::req_t> b)
{
  rpc::tst2_prog_1::tst2_mget_srv_t<amt::req_t> srv (b);
  const tst2_mget_arg_t *arg = srv.getarg ();
  ptr<tst2_mget_res_t> res = srv.alloc_res (ADB_OK);

  if (!_q_mget->execute (arg->sleep_msec, arg->lim)) {
    res->set_status (ADB_EXECUTE_ERROR);
    TWARN("mget error: " << _q_mget->error ());
  } else {
    adb_status_t s;
    tst2_data_t dat;
    while ((s = _q_mget->fetch (&dat.d, &dat.i, &dat.pk, &dat.d2)) == ADB_OK) {
      res->rows->push_back (dat);
    }
    if (s != ADB_NOT_FOUND) {
      TWARN("mget error: " << _q_mget->error ());
      res->set_status (s);
    }
  }
  srv.reply (res);
}

//-----------------------------------------------------------------------

void
tst2_srv_t::get (ptr<amt::req_t> b)
{
  rpc::tst2_prog_1::tst2_get_srv_t<amt::req_t> srv (b);
  const tst2_get_arg_t *arg  = srv.getarg ();
  adb_status_t rc;

  ptr<tst2_get_res_t> res = srv.alloc_res (ADB_OK);
  
  if (!_q_get->execute (*arg)) {
    rc = ADB_EXECUTE_ERROR;
  } else {
    rc = _q_get->fetch (&res->dat->pk, &res->dat->d, &res->dat->i,
			&res->dat->d2);
  }

  if (rc != ADB_OK) {
    res->set_status (rc);
    str es;
    if ((es = _q_get->error ())) 
      TWARN(es);
  }

  srv.reply (res);
}

//-----------------------------------------------------------------------

void
tst2_srv_t::put (ptr<amt::req_t> b)
{
  rpc::tst2_prog_1::tst2_put_srv_t<amt::req_t> srv (b);
  const tst2_put_arg_t *arg = srv.getarg ();
  ptr<adb_status_t> res = srv.alloc_res (ADB_OK);

  if (!_q_put->execute (arg->key, arg->data.d, arg->data.i, arg->data.d2)) {
    *res = ADB_EXECUTE_ERROR;
    str es;
    if ((es = _q_put->error ())) 
      TWARN("put error: " << es);
  }

  b->reply (res);
}

//-----------------------------------------------------------------------

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
#ifdef __clang_analyzer__
  // Silences a value never read warning
  [[clang::unused]] auto const _unused = argv;
#endif

  if (argc != 0)
    usage ();

  mtd_thread_typ_t method = MTD_NONE;
#if HAVE_PTHREADS
  method = MTD_PTHREAD;
#else
# if HAVE_PTH
  method = MTD_PTH;
# endif
#endif
  
  ssrv_t *s = New ssrv_t (wrap (&tst2_srv_t::alloc, mysql_sth_method), 
			  tst2_prog_1, method, tcnt, maxq);

  json_XDR_dispatch_t::enable ();
  
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
