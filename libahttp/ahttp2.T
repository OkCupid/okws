// -*-c++-*-
/* $Id: ahttp.C 3972 2009-01-21 00:20:21Z max $ */

#include "ahttp.h"

//-----------------------------------------------------------------------

tamed void 
ahttpcon::send2 (const strbuf &b, event<ssize_t>::ref ev)
{
  tvars {
    ssize_t ret (0);
    suio *uio (b.tosuio ());
    int rc;
    holdvar ptr<ahttpcon> hold (mkref (_self));
    size_t pre, post;
    rendezvous_t<bool> rv (__FILE__, __LINE__);
    bool eof;
    str ip;
    const char *ipc;
  }

  _state = AHTTPCON_STATE_SEND2;

  ip = get_remote_ip ();
  if (ip) { ipc = ip.cstr(); }
  else { ipc = "<none>"; }

  seteofcb (mkevent (rv, true));
  
  do {

    if (fd < 0) {
      warn ("ahttpcon::send2: write not possible due to EOF (%s)\n", ipc);
      ret = -1;

    } else {
     
      pre = uio->resid ();
      rc = uio->output (fd);
      post = uio->resid ();

      // update accounting whether we're done or not...
      if (rc > 0) {
	assert (pre > post);
	ret += (pre - post);
      }

      if (!post) {

	// noop: we're all done and will be exitting the loop...

      } else if (rc >= 0) {

	assert (!wcbset);
	wcbset = true;
	fdcb (fd, selwrite, mkevent (rv, false));
	twait (rv, eof);
	if (fd >= 0) {
	  wcbset = false;
	  fdcb (fd, selwrite, NULL);
	}

	if (eof) {
	  warn ("ahttpcon::send2: EOF while waiting for write (%s)\n", ipc);
	  ret = -1;
	}

      } else {

	warn ("ahttpcon::send2: For fd=%d: error in write: %m (%s)\n", fd, ipc);
	ret = -1;
      }
    }
  } while (ret >= 0 && post);

  rv.cancel ();
  
  ev->trigger (ret);
}

//-----------------------------------------------------------------------
