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

#include "async.h"
#include "arpc.h"
#include "ezdb_prot.h"
#include "ezdb_field.h"
#include "ezdb_serve.h"
#include "pslave.h"

#include "okwsconf.h"
#ifdef HAVE_MYSQL
#include "amysql.h"
#include "mystmt.h"
#include "web.h"
#include "crypt.h"

//-----------------------------------------------------------------------

// 2-minute timeout on query sets!
ezdb::srv_t ez_guts (120);

//-----------------------------------------------------------------------

class ezdb_srv_t : public amysql_thread2_t {
public:
  ezdb_srv_t (dbparam_t d, mtd_thread_arg_t *a) : 
    amysql_thread2_t (a, 0), err (false), _dbp (d) {}
  bool init ();
  void dispatch (ptr<amt::req_t> sbp);
  void handle_execute (ptr<amt::req_t> sbp);
  static mtd_thread_t *alloc (dbparam_t d, mtd_thread_arg_t *arg)
  { return New ezdb_srv_t (d, arg); }
protected:
  bool err;
  dbparam_t _dbp;
};

//-----------------------------------------------------------------------

class ezdb_ssrv_t : public ssrv_t {
public:
  ezdb_ssrv_t (int tcnt, int maxq, dbparam_t d);
  bool skip_db_call (svccb *sbp);
  void handle_fetch (svccb *sbp);
  void handle_finish (svccb *sbp);
  dbparam_t _dbp;
};

//-----------------------------------------------------------------------

bool
ezdb_srv_t::init ()
{
  bool rc = mysql.connect (_dbp);
  set_safe (false); // EZDB needs to be able to prepare whenever...
  return rc;
}

//-----------------------------------------------------------------------

void
ezdb_ssrv_t::handle_finish (svccb *sbp)
{
  rpc::ezdb_prog_1::ezdb_finish_srv_t<svccb> srv (sbp);
  const sth_id_t *arg = srv.getarg ();
  ptr<adb_status_t> res = srv.alloc_res ();
  ez_guts.handle_finish (*arg, res);
  srv.reply (res);
}

//-----------------------------------------------------------------------

void
ezdb_ssrv_t::handle_fetch (svccb *sbp)
{
  rpc::ezdb_prog_1::ezdb_fetch_srv_t<svccb> srv (sbp);
  const ezdb_fetch_arg_t *arg = srv.getarg ();
  ptr<ezdb_fetch_res_t> res = srv.alloc_res ();
  ez_guts.handle_fetch (arg, res);
  srv.reply (res);
}

//-----------------------------------------------------------------------

void 
ezdb_srv_t::handle_execute (ptr<amt::req_t> sbp)
{
  rpc::ezdb_prog_1::ezdb_execute_srv_t<amt::req_t> srv (sbp);
  const ezdb_execute_arg_t *arg = srv.getarg ();
  ptr<ezdb_execute_res_t> res = srv.alloc_res ();
  ez_guts.handle_execute (this, arg, res);
  reply (res);
}

//-----------------------------------------------------------------------

void
ezdb_srv_t::dispatch (ptr<amt::req_t> sbp)
{
  u_int p = sbp->proc ();
  switch (p) {
  case EZDB_NULL:
    reply (NULL);
    break;
  case EZDB_EXECUTE:
    handle_execute (sbp);
    break;
  default:
    reject ();
    break;
  }
}

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " OPTIONS\n"
	<< " OPTIONS are:\n"
	<< "   -b <port>  the port I should bind to\n"
	<< "   -t <cnt>   the number of threads to create\n"
	<< "   -q <sz>    the maximum size of the request queue\n"
	<< "   -h <host>  the database host (default=localhost)\n"
	<< "   -P <port>  the database port (default=3306)\n"
	<< "   -p <pw>    the database PW to use (default='')\n"
	<< "   -d <db>    the database name\n"
	<< "   -u <user>  the database user to connect as\n";
  exit (1);
}

//-----------------------------------------------------------------------

ezdb_ssrv_t::ezdb_ssrv_t (int tcnt, int maxq, dbparam_t d)
  : ssrv_t (wrap (&ezdb_srv_t::alloc, d), ezdb_prog_1, MTD_PTH, tcnt, maxq), 
    _dbp (d) {}

//-----------------------------------------------------------------------

bool 
ezdb_ssrv_t::skip_db_call (svccb *sbp)
{
  bool ret = true;
  switch (sbp->proc ()) {
  case EZDB_FETCH:
    handle_fetch (sbp);
    break;
  case EZDB_FINISH:
    handle_finish (sbp);
    break;
  default:
    ret = false;
    break;
  }
  return ret;
}

//-----------------------------------------------------------------------

static void
start_server (int argc, char *argv[])
{
  int tcnt = 10;
  int maxq = 1000;

  int ch;
  dbparam_t dbp;
  dbp._port = 3306; // MySQL's port
  dbp._host = "localhost";

  int port = EZDBD_PORT;

  while ((ch = getopt (argc, argv, "t:q:h:P:d:u:p:b:")) != -1)
    switch (ch) {
    case 'b':
      if (!convertint (optarg, &port)) {
	warn << "Cannot convert port='" << optarg << "' to an int\n";
	usage ();
      }
      break;
    case 't':
      if (!convertint (optarg, &tcnt)) usage ();
      break;
    case 'q':
      if (!convertint (optarg, &maxq)) usage ();
      break;
    case 'h':
      dbp._host = optarg;
      break;
    case 'u':
      dbp._user = optarg;
      break;
    case 'P':
      if (!convertint (optarg, &dbp._port)) {
	warn << "Cannot convert database-port='" << optarg << "' to int\n";
	usage ();
      }
      break;
    case 'p':
      dbp._pw = optarg;
      break;
    case 'd':
      dbp._database = optarg;
      break;
    default:
      usage ();
    }

  if (!dbp._user) { warn << "Need a database user\n"; usage ();  }
  if (!dbp._database) { warn << "Need a database\n"; usage (); }

  argc -= optind;
  argv += optind;

#ifdef __clang_analyzer__
  // Silences a value never read warning
  [[clang::unused]] auto const _unused = argv;
#endif

  if (argc != 0)
    usage ();
  
  ssrv_t *s = New ezdb_ssrv_t (tcnt, maxq, dbp);
  
  // turn off all warning messages
  s->mtd->set_quiet (true);
  
  if (!pub_server (wrap (s, &ssrv_t::accept), EZDBD_PORT))
    fatal << "Cannot bind to port " << EZDBD_PORT << "\n";
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

