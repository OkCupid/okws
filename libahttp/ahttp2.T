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
  }

  do {
    if (fd < 0) {
      warn ("write not possible due to EOF\n");
      ret = -1;
    } else {
      rc = uio->output (fd);
      if (rc > 0) {
	ret += rc;
      } else if (rc < 0 && errno == EAGAIN) {
	assert (!wcbset);
	wcbset = true;
	twait { fdcb (fd, selwrite, mkevent ()); }
	if (fd >= 0) {
	  wcbset = false;
	  fdcb (fd, selwrite, NULL);
	}
      } else if (rc < 0) {
	warn ("For fd=%d: error in write: %m\n", fd);
	rc = -1;
      }
    }
  } while (ret >= 0 && uio->resid ());
  
  ev->trigger (ret);

}

//-----------------------------------------------------------------------
