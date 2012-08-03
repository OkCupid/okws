// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "tame.h"
#include "tame_io.h"
#include "parseopt.h"

static void usage (void)
{
  warnx << "usage: " << progname << " <port>\n";
  exit (1);
}

#define BUFSZ 0x10400
//#define BUFSZ 0x8000
//#define BUFSZ 1024

static const char * resp[] = 
  { "HTTP/1.1 405\n",
    "Date: Mon, 12 May 2008 14:12:24 GMT\n",
    "Content-Type: text/html\n",
    "Connection: close\n",
    "Cache-control: private\n",
    "Server: OKWS/2.0.0pre1\n",
    "\n",
    "<html>\n",
    "<head>\n",
    "<title>405 Method Not Allowed</title>\n",
    "</head>\n",
    "<body>\n",
    "<h1>Error 405 Method Not Allowed</h1><br><br>\n",
    "<hr>\n",
    "<i>okws/2.0.0pre1 Server at reptile.maxk.org Port 8081</i>\n",
    "<br>\n",
    "</body>\n",
    "</html>\n",
    NULL };

str resp_str;

static str 
make_resp ()
{
  if (!resp_str) {
    strbuf b;
    const char **cp;
    for (cp = resp; *cp; cp++) {
      b << *cp;
    }
    resp_str = b;
  }
  return resp_str;
}

void
fiddle_with_send (int fd)
{
  if (0) {
  int opt = 1;
  socklen_t optlen = sizeof (opt);
  int rc = getsockopt (fd, SOL_SOCKET, SO_SNDLOWAT, &opt, &optlen);
  warn ("getlowat %d, %d\n", rc, opt);
  opt = 1;
  struct linger l;
  getsockopt (fd, SOL_SOCKET, SO_LINGER, (char *)&l, &optlen);
  warn ("linger: %d, %d\n", l.l_onoff, l.l_linger);
  l.l_onoff = 1;
  l.l_linger = 1;
  rc = setsockopt (fd, SOL_SOCKET, SO_LINGER, (char *)&l, optlen);
  if (rc <0) {
    warn ("setsockopt: %m\n");
  }
  }

  shutdown (fd, SHUT_RD);
  warn << "trying 1-way shutdown!\n";

  char buf[BUFSZ];
  ssize_t rc;
  while ((rc = read (fd, buf, BUFSZ)) > 0) {}
  warn << "done read flush; rc=" << rc << "\n";

}  

tamed static void
handle_one (int fd, evv_t ev)
{
  tvars {
    char buf[BUFSZ];
    int rc;
    str s;
  }
  make_async (fd);
  tcp_nodelay (fd);

  do {
    twait { tame::waitread (fd, mkevent ()); }
    rc = read (fd, buf, BUFSZ);
    warn << "Read " << rc << " bytes\n";
  } while (0);
  warn << "done reading!\n";
  s = make_resp ();
  //fiddle_with_send (fd);
  rc = write (fd, s.cstr (), s.len ());
  warn << "Wrote " << rc << " bytes\n";
  close (fd);
  ev->trigger ();
}
#undef BUFSZ

tamed static void
main_T (int port)
{
  tvars {
    int sfd;
    sockaddr_in sin;
    socklen_t sinlen (sizeof (sin));
    int nfd;
    bool go (true);
  }
  sfd = inetsocket (SOCK_STREAM, port);
  listen (sfd, 300);

  while (go) {
    twait { tame::waitread (sfd, mkevent ()); }
    nfd = accept (sfd, reinterpret_cast<sockaddr *> (&sin), &sinlen);
    if (nfd < 0) {
      warn ("accept: %m\n");
    } else {
      warn ("accept OK!\n");
      twait { handle_one (nfd, mkevent ()); }
    }
  }
}

int
main (int argc, char *argv[])
{
  int port=0;
  setprogname (argv[0]);
  if (argc != 2 || !convertint (argv[1], &port))
    usage ();
  

  main_T (port);
  amain ();
}
