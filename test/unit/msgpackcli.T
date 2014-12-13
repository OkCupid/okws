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

class client_t {
public:
  client_t ();
  bool config (int argc, char **argv);
  void connect (evb_t ev, CLOSURE);
  void run (evb_t ev, CLOSURE);
private:
  void run1 (evi_t ev, CLOSURE);
  void do_call (str cmd, evi_t ev, CLOSURE);
  
  int _fd;
  int _port;
  str _host;
  str _prog;
  ptr<pub3::msgpack::axprt> _x;
  ptr<pub3::msgpack::aclnt> _cli;
};

//-----------------------------------------------------------------------

client_t::client_t () : _fd (-1), _port (40001) {}

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: " << progname << " [-p <port>] [-h <host>] [-g <prog>]\n";
}

//-----------------------------------------------------------------------

bool 
client_t::config (int argc, char **argv) 
{
  int ch;
  bool ret = true;
  while ((ch = getopt (argc, argv, "g:p:h:")) != -1) {
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
    case 'g':
      _prog = optarg;
      break;
    default:
      usage ();
      ret = false;
      break;
    }
  }
  if (!_host || _host.len () == 0 || _host == "-" || _host == "localhost") {
    _host = "127.0.0.1";
  }
  return ret; 
}

//-----------------------------------------------------------------------

tamed void 
client_t::connect (evb_t ev) 
{ 
  tvars {
    bool ret (true);
  }
  twait { tcpconnect (_host, _port, mkevent (_fd)); }
  if (_fd < 0) {
    ret = false;
    warn << "cannot connect to host " << _host << ":" << _port << "\n";
  } else {
    _x = pub3::msgpack::axprt::alloc (_fd);
    _cli = pub3::msgpack::aclnt::alloc (_x, _prog);
  }
  ev->trigger (ret); 
}

//-----------------------------------------------------------------------

tamed void
client_t::do_call (str cmd, evi_t ev)
{
  tvars {
    ptr<pub3::expr_t> x;
    ptr<pub3::expr_list_t> l;
    pub3::msgpack::callres_t res;
    int rc (1);
    str mthd;
  }
  warn << "RPC(" << cmd << ") ->\n";
  if (!(x = pub3::json_parser_t::parse (cmd))) {
      warn << "parse of JSON failed on string: " << cmd << "\n";
  } else if (!(l = x->to_list ()) || l->size () != 2 ||
             !(mthd = (*l)[0]->to_str ()) || !mthd.len ()) {
      warn << "parse of JSON didn't yield a list with 2 elements: [mthd,arg]\n";
  } else {
      twait { _cli->call (mthd, (*l)[1], mkevent (res)); }
      if (res.err_code) {
          if (res.err_obj) {
              str m = res.err_obj->to_str ();
          }
      } else if (res.res) {
          str o = res.res->to_str ();
          warn << "  " << o << "\n";
      }
  }

  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
client_t::run (evb_t ev)
{
  tvars {
    bool ok (true);
    bool go (true);
    int rc;
  }

  while (go) {
    twait { run1 (mkevent (rc)); }
    if (rc == 0) { go = false; }
    else if (rc < 0) { go = ok = false; }
  }

  ev->trigger (ok);
}

//-----------------------------------------------------------------------

tamed void 
client_t::run1 (evi_t ev) 
{
  tvars {
    str line;
    int rc, trc;
  }
  twait { ain->readline (mkevent (line, trc)); }
  if (!line) { rc = 0; }
  else if (trc) { warn ("error condition hit %m"); rc = -1; }
  else {
    twait { do_call (line, mkevent (rc)); }
  }
  ev->trigger (rc); 
}

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
    if (ok) { twait { cli.run (mkevent (ok)); } }
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
