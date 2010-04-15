// -*-c++-*-

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

#include "ahutil.h"
#include "okd.h"
#include "pslave.h"
#include "okprot.h"
#include "okdbg.h"

//-----------------------------------------------------------------------

tamed void
okd_t::shutdown_wait (okd_t::shutdown_rv_t *rv, evb_t ev)
{
  tvars {
    okch_t *ch;
    ptr<bool> df;
    bool timeout (false);
  }
  while (servtab.size () && !timeout) { 
    twait (*rv, ch, df);
    if (!ch) timeout = true;
    else if (!*df) { ch->killed (); }
  }
  ev->trigger (timeout);
}

//-----------------------------------------------------------------------

static void set_timeout (okd_t::shutdown_rv_t *rv)
{ 
  okch_t *n1 = NULL;
  ptr<bool> n2;
  delaycb (ok_shutdown_timeout, 0, mkevent (*rv, n1, n2));
}

//-----------------------------------------------------------------------

tamed void
okd_t::shutdown_T (int sig)
{
  tvars {
    okd_t::shutdown_rv_t rv (__FILE__, __LINE__);
    bool timeout (false);
  }

  // child can get parent's SIGTERM signal as well as parent's 
  // EOF on the fdsource
  if (sdflag)
    return;

  if (OKDBG2(OKD_SHUTDOWN)) {
    strbuf b;
    b << "commencing shutdown sequence with signal=" << sig ;
    okdbg_warn (CHATTER, b);
  }

  sdflag = true;
  stop_listening ();
  xtab.shutdown ();

  OKDBG3(OKD_SHUTDOWN, CHATTER, "sending soft KILL to all services");

  OKDBG4(OKD_SHUTDOWN, CHATTER,
	 "debug: setting shutdown timer: %d\n", 
	 ok_shutdown_timeout);

  set_timeout (&rv);
  kill_srvcs (OK_SIG_SOFTKILL, &rv);
  twait { shutdown_wait (&rv, mkevent (timeout)); }
 
  if (timeout) {
    warn << "shutdown timeout; sending hard KILL to all services.\n";

    set_timeout (&rv);
    kill_srvcs (OK_SIG_HARDKILL, &rv);
    twait { shutdown_wait (&rv, mkevent (timeout)); }

    if (timeout) {
      warn << "aborting all unresponsive services\n";
      kill_srvcs (OK_SIG_ABORT, &rv);
    }
  }

  warn << "shutdown complete\n";
  exit (0);
}

//-----------------------------------------------------------------------

void
okd_t::kill_srvcs (oksig_t sig, shutdown_rv_t *rv)
{
  okch_t *ch;
  vec<okch_t *> all;
  servtab.dump (&all);
  for (size_t i = 0; i < all.size(); i++) {
    ch = all[i];
    ch->shutdown (sig, mkevent (*rv, ch, ch->get_destroyed_flag ()));
  }
}


//-----------------------------------------------------------------------

