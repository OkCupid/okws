
#include "ahutil.h"
#include "okd.h"
#include "pslave.h"
#include "okprot.h"
#include "xpub.h"

static void shutdown_srvc (oksig_t g, okd_t *d, okch_t *s) 
{ s->shutdown (g, wrap (d, &okd_t::kill_srvcs_cb)); }

void
okd_t::shutdown (int sig)
{
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
  warn << "shutdown timeout; sending hard KILL to all services.\n";
  kill_srvcs (OK_SIG_HARDKILL);
}

void
okd_t::shutdown2 ()
{
  sd2 = true;
  if (dcb) {
    timecb_remove (dcb);
    dcb = NULL;
  }

  sdcbcnt = 0;
  if (pubd) sdcbcnt++;
  if (logd) sdcbcnt++;

  if (logd) logd->kill (wrap (this, &okd_t::shutdown_cb1));
  if (pubd) pubd->kill (wrap (this, &okd_t::shutdown_cb1));

  if (!sdcbcnt)
    shutdown_cb1 ();
}

void
okd_t::shutdown_cb1 ()
{
  if (!--sdcbcnt) 
    shutdown3 ();
}

void
okd_t::shutdown3 ()
{
  delete this;
  warn << "shutdown complete\n";
  exit (0);
}
