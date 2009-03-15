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
  }

  do {

    if (fd < 0) {
      warn ("write not possible due to EOF\n");
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
	twait { fdcb (fd, selwrite, mkevent ()); }
	if (fd >= 0) {
	  wcbset = false;
	  fdcb (fd, selwrite, NULL);
	}

      } else {

	warn ("For fd=%d: error in write: %m\n", fd);
	ret = -1;
      }
    }
  } while (ret >= 0 && post);
  
  ev->trigger (ret);

}

//-----------------------------------------------------------------------
