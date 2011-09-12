// -*-c++-*-
/* $Id: tst2.g 1007 2005-09-11 21:45:33Z max $ */

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

#include "ok.h"
#include "okcgi.h"
#include "parseopt.h"
#include "pub.h"
#include <unistd.h>
#include "tst2_prot.h"
#include "tame_pipeline3.h"
#include "crypt.h"
#include "web.h"

//-----------------------------------------------------------------------

class runner_t {
public:
  runner_t () 
    : _host ("localhost"),
      _port (TST2_PORT),
      _num (1000),
      _concur (20),
      _fd (-1),
      _rows (30),
      _sleep (10),
      _put_freq (10) {}

  int config (int argc, char **argv);
  void init (evi_t ev, CLOSURE);
  void run (evi_t ev, CLOSURE);
protected:
  void do_put (size_t i, evi_t ev, CLOSURE);
  void do_get (size_t i, evi_t ev, CLOSURE);
private:
  str _host;
  int _port;
  size_t _num, _concur;
  int _fd;
  size_t _rows;
  time_t _sleep;
  size_t _put_freq;
  ptr<axprt_stream> _x;
  ptr<aclnt> _cli;
  ptr<pipeline3::runner_t> _pipeline;
};

//-----------------------------------------------------------------------

static void 
usage ()
{
  warnx << "usage: " << progname << " [-h <host>] [-p <port>] "
	<< "[-n <num-total>] [-c <num-concur>] [-r <rows>] "
	<< "[-s <sleep-msec>] [-f <put-freq>]\n";
}

//-----------------------------------------------------------------------

int
runner_t::config (int argc, char **argv)
{
  int ch;
  int rc (0);
  while ((ch = getopt (argc, argv, "h:p:n:c:r:f:s:")) != -1) {
    switch (ch) {
    case 'h':
      _host = optarg;
      break;
    case 'p':
      if (!convertint (optarg, &_port)) {
	warn << "Cannot convert '" << optarg << "' to an int\n";
	rc = -1;
      }
      break;
    case 'n':
      if (!convertint (optarg, &_num)) {
	warn << "Cannot convert '" << optarg << "' to an int\n";
	rc = -1;
      }
      break;
    case 'c':
      if (!convertint (optarg, &_concur)) {
	warn << "Cannot convert '" << optarg << "' to an int\n";
	rc = -1;
      }
      break;
    case 'r':
      if (!convertint (optarg, &_rows)) {
	warn << "Cannot convert '" << optarg << "' to an int\n";
	rc = -1;
      }
      break;
    case 'f':
      if (!convertint (optarg, &_put_freq)) {
	warn << "Cannot convert '" << optarg << "' to an int\n";
	rc = -1;
      }
      break;
    case 's':
      if (!convertint (optarg, &_sleep)) {
	warn << "Cannot convert '" << optarg << "' to an int\n";
	rc = -1;
      }
      break;
    default:
      rc = -1;
      break;
    }
  }
  if (rc != 0) {
    usage ();
  }
  return rc;
}

//-----------------------------------------------------------------------

tamed void
runner_t::do_put (size_t i, evi_t ev)
{
  tvars { 
    int rc (0); 
    tst2_put_arg_t arg;
    adb_status_t res;
    clnt_stat err;
    okdate_t now (okwstime ());
  }
  arg.key = strbuf ("0x%" PRIx64, rnd.gethyper ());
  warn << "putting key = " << arg.key << "\n";
  arg.data.i = rnd.getword ();
  arg.data.pk = rnd.getword ();
  now.to_xdr (&arg.data.d);
  now.to_xdr (&arg.data.d2);
  twait { rpc::tst2_prog_1::tst2_put (_cli, &arg, &res, mkevent (err)); }

  if (err) {
    warn << "XXX DB error in put: " << err << "\n";
    rc = -1;
  } else if (res != ADB_OK) {
    warn << "XXX put failed with status=" << int (res) << "\n";
    rc =-1;
  } else {
    warn << "iter[" << i << "]: PUT\n";
  }

  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
runner_t::do_get (size_t i, evi_t ev)
{
  tvars {
    int rc (0); 
    tst2_mget_arg_t arg;
    tst2_mget_res_t res;
    clnt_stat err;
  }

  if (_sleep) {
    arg.sleep_msec = rnd.getword () % ( 2 * _sleep);
  } else {
    arg.sleep_msec = 0;
  }
  arg.lim = _rows;

  twait { rpc::tst2_prog_1::tst2_mget (_cli, &arg, &res, mkevent (err)); }
  if (err) {
    warn << "XXX DB error in get: " << err << "\n";
    rc = -1;
  } else if (res.status != ADB_OK) {
    warn << "XXX get failed with status=" << int (res.status) << "\n";
    rc = -1;
  } else {
    warn << "iter[" << i << "]: got " << res.rows->size () << " rows\n";
  }

  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
runner_t::run (evi_t ev)
{
  tvars {
    int trc (0), rc (0);
    size_t i (0);
  }

  while (i < _num) {
    twait { _pipeline->queue_for_takeoff (mkevent ()); }
    if (i % _put_freq == 0) {
      twait { do_put (i, _pipeline->mkev (trc)); }
    } else {
      twait { do_get (i, _pipeline->mkev (trc)); }
    }
    if (trc != 0) { rc = trc; }
    i++;
  }
  twait { _pipeline->flush (mkevent ()); }

  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
runner_t::init (evi_t ev)
{
  tvars {
    int rc (0);
  }
  twait { tcpconnect (_host, _port, mkevent (_fd)); }
  if (_fd < 0) {
    warn ("cannot connect to %s:%d: %m\n", _host.cstr (), _port);
    rc = -1; 
  } else {
    _x = axprt_stream::alloc (_fd, 0x1000000);
    _cli = aclnt::alloc (_x, tst2_prog_1);
    _pipeline = New refcounted<pipeline3::runner_t> 
      (New refcounted<pipeline3::passive_control_t> (_concur));
  }

  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    runner_t r;
    int rc (0);
  }
  random_init ();
  rc = r.config (argc, argv);
  if (rc == 0) { twait { r.init (mkevent (rc)); } }
  if (rc == 0) { twait { r.run (mkevent (rc)); } }
  exit (rc);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}

//-----------------------------------------------------------------------
