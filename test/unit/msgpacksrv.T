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
#include "aios.h"
#include "pub3.h"
#include "pub3parse.h"
#include "pub3msgpack.h"
#include "pub3msgpackrpc.h"

#include "amysql.h"

//-----------------------------------------------------------------------

class my_server_t;

class my_server_con_t : public pub3::msgpack::server_con_t {
public:
  my_server_con_t (ptr<my_server_t> parent, 
		   ptr<pub3::msgpack::axprt> x, str prog)
    : pub3::msgpack::server_con_t (parent, x, prog) {}
  void handle_call (pub3::msgpack::svccb b);
};

//-----------------------------------------------------------------------

class my_server_t : public pub3::msgpack::server_t {
public:
  my_server_t () : pub3::msgpack::server_t (40001) {}
  bool config (int argc, char **argv);
  ptr<pub3::msgpack::server_con_t> make_new_con (ptr<pub3::msgpack::axprt> x);
private:
  str _prog;
};

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " [-p <port>] [-g <prog>]\n";
}

//-----------------------------------------------------------------------

bool 
my_server_t::config (int argc, char **argv) 
{
  int ch;
  bool ret = true;
  while ((ch = getopt (argc, argv, "g:p:")) != -1) {
    switch (ch) {
    case 'p':
      if (!convertint (optarg, &_port)) {
	warn << "cannot convert port to int: " << optarg << "\n";
	ret = false;
      }
      break;
    case 'g':
      _prog = optarg;
      break;
    default:
      usage ();
      ret = false;
      break;
    }
  }
  return ret; 
}

//-----------------------------------------------------------------------

ptr<pub3::msgpack::server_con_t>
my_server_t::make_new_con (ptr<pub3::msgpack::axprt> x)
{
  ptr<my_server_t> s = mkref (this);
  ptr<my_server_con_t> c = 
    New refcounted<my_server_con_t> (s, x, _prog);
  return c;
}

//-----------------------------------------------------------------------

void
my_server_con_t::handle_call (pub3::msgpack::svccb b)
{
  
}

//-----------------------------------------------------------------------

static void
main2 (int argc, char **argv)
{
  ptr<my_server_t> srv = New refcounted<my_server_t> ();
  bool ok = true;

  if (!srv->config (argc, argv)) { ok = false; }
  else if (!srv->bind ()) { ok = false; }
  if (!ok) { exit (-2); }
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
