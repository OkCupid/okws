
#include "ahutil.h"
#include "okd.h"
#include "pslave.h"
#include "okprot.h"
#include "xpub.h"
#include "authtok.h"

static void shutdown_srvc (oksig_t g, okd_t *d, okch_t *s) 
{ s->shutdown (g, wrap (d, &okd_t::kill_srvcs_cb)); }

void
okd_t::shutdown (int sig)
{
  // child can get parent's SIGTERM signal as well as parent's 
  // EOF on the fdsource
  if (sdflag)
    return;

  warn << "commencing shutdown sequence with signal=" << sig << "\n";

  sdflag = true;
  sd2 = false;
  stop_listening ();
  warn << "sending soft KILL to all services.\n";
  kill_srvcs (OK_SIG_SOFTKILL);
}

void
okd_t::kill_srvcs (oksig_t sig)
{
  u_int i = servtab.size ();
  if (i > 0) {
    servtab.traverse (wrap (shutdown_srvc, sig, this));
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
  if (++sdattempt > 3) {
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
