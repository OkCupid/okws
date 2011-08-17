// -*-c++-*-
/* $Id: eztst.g 1007 2005-09-11 21:45:33Z max $ */

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
#include "pub.h"
#include <unistd.h>

#include "amysql.h"

//-----------------------------------------------------------------------

class client_t {
public:
  client_t ();
  bool config (int argc, char **argv);
  void connect (evb_t ev, CLOSURE);
  void run (evb_t ev, CLOSURE);
private:
  int _port;
  str _host;
};

//-----------------------------------------------------------------------

client_t::client_t () : _port (40001), _host ("localhost") {}

//-----------------------------------------------------------------------

bool 
client_t::config (int argc, char **argv) 
{
  int ch;
  bool ret = true;
  while ((ch = getopt (argc, argv, "d:h:")) != -1) {
    switch (ch) {
    case 'h':
      _host = optarg;
      break;
    case 'p':
      if (!convertint (optarg, &_port)) {
	warn << "cannot convert port to int: " << optarg << "\n";
	ret = false;
      }
      break;
    default:
      break;
    }
  }
  return ret; 
}

//-----------------------------------------------------------------------

tamed void client_t::connect (evb_t ev) { ev->trigger (false); }
tamed void client_t::run (evb_t ev) { ev->trigger (false); }

//-----------------------------------------------------------------------

tamed static void
main2 (int argc, char **argv)
{
  tvars {
    client_t cli;
    bool ok;
  }

  if (!cli.config (argc, argv)) { ok = false; }
  else {
    twait { cli.connect (mkevent (ok)); }
    if (!ok) { twait { cli.run (mkevent (ok)); } }
  }
  exit (ok ? 0 : -2);
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
