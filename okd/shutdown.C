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

#include "ahutil.h"
#include "okd.h"
#include "pslave.h"
#include "okprot.h"
#include "xpub.h"
#include "okdbg.h"

static void shutdown_srvc (oksig_t g, okd_t *d, okch_t *s) 
{ s->shutdown (g, wrap (d, &okd_t::kill_srvcs_cb)); }

void
okd_t::shutdown (int sig)
{
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
  sd2 = false;
  stop_listening ();

  OKDBG3(OKD_SHUTDOWN, CHATTER, "sending soft KILL to all services");

  kill_srvcs (OK_SIG_SOFTKILL);
}

void
okd_t::kill_srvcs (oksig_t sig)
{
  u_int i = servtab.size ();
  if (i > 0) {
    servtab.traverse (wrap (shutdown_srvc, sig, this));
    
    OKDBG4(OKD_SHUTDOWN, CHATTER,
	   "debug: setting shutdown timer: %d\n", 
	   ok_shutdown_timeout);

    dcb = delaycb (ok_shutdown_timeout, 0, 
		   wrap (this, &okd_t::shutdown_retry));
  } else {
    kill_srvcs_cb ();
  }
}

void
okd_t::kill_srvcs_cb ()
{
  if (!servtab.size () && !sd2)
    shutdown2 ();
}

void
okd_t::shutdown_retry ()
{
  dcb = NULL;
  if (++sdattempt > int (ok_shutdown_retries)) {
    warn << "aborting all unresponsive services\n";
    kill_srvcs (OK_SIG_ABORT);
  } else {
    warn << "shutdown timeout; sending hard KILL to all services.\n";
    kill_srvcs (OK_SIG_HARDKILL);
  }
}

void
okd_t::shutdown2 ()
{
  sd2 = true;
  if (dcb) {
    timecb_remove (dcb);
    dcb = NULL;
  }


  // no need to disconnect from oklogd or pubd explicitly/ that will
  // happen when we delete this.
  delete this;
  warn << "shutdown complete\n";
  exit (0);
}
