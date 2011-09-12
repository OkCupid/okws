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

//-----------------------------------------------------------------------

class runner_t {
public:
  runner_t () 
    : _host ("localhost"),
      _port (TST2_PORT),
      _num (1000),
      _concur (20) {}

  int config (int argc, char **argv);
  void init (evi_t ev, CLOSURE);
  void run (evi_t ev, CLOSURE);
private:
  str _host;
  int _port;
  size_t _num, _concur;
};

//-----------------------------------------------------------------------

static void 
usage ()
{
  warnx << "usage: " << progname << " [-h <host>] [-p <port>] "
	<< "[-n <num-total>] [-c <num-concur>]\n";
}

//-----------------------------------------------------------------------

int
runner_t::config (int argc, char **argv)
{
  int ch;
  int rc (0);
  while ((ch = getopt (argc, argv, "h:p:n:c:")) != -1) {
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
runner_t::run (evi_t ev)
{
  tvars {
    int rc (0);
  }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
runner_t::init (evi_t ev)
{
  tvars {
    int rc (0);
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
