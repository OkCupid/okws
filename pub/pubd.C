/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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

#include "pub.h"
#include "pub_parse.h"
#include "xpub.h"
#include <unistd.h>
#include <stdlib.h>
#include "pubd.h"
#include "parseopt.h"
#include "pslave.h"

// global variables used to time out connection if no contact from parent
pslave_status_t slave_status;
bool parent_connect;
bool primary;

static void
client_accept (ptr<axprt_stream> x)
{
  if (!x)
    fatal ("EOF from parent process.\n");
  parent_connect = true;
  vNew pubserv_t (x, primary && slave_status == PSLAVE_SLAVE);
  primary = false;
}

static void 
pubshutdown (int sig)
{
  warn << "Caught signal: " << sig << "; shutting down\n";
  exit (0);
}

static void
set_signals ()
{
  sigcb (SIGTERM, wrap (pubshutdown, SIGTERM));
  sigcb (SIGINT,  wrap (pubshutdown, SIGINT));
}

static void
usage ()
{
  warn << "usage: pubd [-p port] [-w]\n";
  exit (1);
}

static void
pubd_slave_cb ()
{
  if (!parent_connect)
    fatal << "exitting; no parent process found\n";
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  u_int port = ok_pubd_port;
  parent_connect = false;
  int ch;
  str jaildir;
  str uname;
  str gname;
  str configfile = ok_pub_config;
  u_int opts = P_DAEMON;
  primary = true;
  const char *e, *v;

  /* need this call to setup global gzip table */
  zinit ();

  /* 
   * set up global pub variables 
   * arg true = this pubd is an exporter.
   */
  pub_parser_t *ppt = pub_parser_t::alloc (true);

  if ((e = getenv ("PUBCONF")) && (v = getenvval (e)) && *v)
    configfile = v;

  while ((ch = getopt (argc, argv, "vp:wj:u:g:f:")) != -1)
    switch (ch) {
    case 'p':
      if (!convertint (optarg, &port))
	usage ();
    case 'w':
      ppt->wss = true;
      break;
    case 'j':
      jaildir = optarg;
      break;
    case 'u':
      uname = optarg;
      break;
    case 'g':
      gname = optarg;
      break;
    case 'f':
      configfile = optarg;
      break;
    case 'v':
      opts |= P_VERBOSE;
      break;
    default:
      usage ();
    }
  ppt->set_opts (opts);

  if (!ppt->parse_config (configfile))
    warn << "pubd running without default variable bindings\n";

  if (port == ok_pubd_port) {
    str ps = ppt->cfg ("PORT");
    if (ps && !convertint (ps, &port))
      port = ok_pubd_port;
  }
  warn ("version %s, pid %d\n", VERSION, int (getpid ()));
  if ((slave_status = pub_slave (wrap (client_accept), port, &slave_status)) 
      == PSLAVE_ERR) {
    warn << port << ": port already in use / can't bind to port\n";
    exit (1);
  }
  setsid ();
  ppt->setprivs (jaildir, uname, gname);
  set_signals ();
  if (slave_status == PSLAVE_SLAVE)
    delaycb (ok_connect_wait, 0, wrap (pubd_slave_cb));
  amain ();
}


