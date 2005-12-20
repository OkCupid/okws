
/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "ahttp.h"

static void
usage ()
{
  fatal << "usage: hsrv <port>\n";
}

static void
readcb (ptr<ahttpcon> x, int i)
{
  if (!i) {
    // EOF received; unregistering callback will cause the object to be
    // deleted
    x->setrcb (NULL);
    return;
  }
  int rc;
  ssize_t len;
  suiolite *uio = x->uio ();
  const char *bp;
  do {
    bp = uio->getdata (&len);
    if (len > 0) {
      rc = write (1, bp, len);
      if (rc > 0) uio->rembytes (rc);
    } else {
      rc = 0;
    }
  } while (rc > 0 || (rc < 0 && errno == EAGAIN));
}

static void
acceptit (ptr<ahttpcon> x)
{
  x->setrcb (wrap (readcb, x));
}

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc > 2)
    usage ();

  int port = 80;
  if (argc == 2 && !convertint (argv[1], &port))
    usage ();
  if (!http_server (wrap (acceptit), port)) 
    fatal << "Cannot bind to port: " << port << "\n";
  amain ();
}
