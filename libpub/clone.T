// -*-c++-*-
/* $Id$ */

#include "okclone.h"
#include "okdbg.h"
#include "tame.h"

void
clone_server_t::clonefd (svccb *b)
{
  int32_t out = 0;
  int fds[2];
  int rc = socketpair (AF_UNIX, SOCK_STREAM, 0, fds);
  if (rc < 0) {
    warn ("socketpair: %m\n");
    out = -1;
  } else {
    out = *b->Xtmpl getarg<int32_t> ();
    _x->sendfd (fds[1], true);
    register_newclient (axprt_unix::alloc (fds[0], ok_axprt_ps));
  }
  b->replyref (out);
}

tamed void 
clone_client_t::clone (evi_t ev)
{
  tvars {
    clnt_stat err;
    int32_t seqno;
    int32_t res;
    int ret (-1);
  }
  seqno = _seqno ++;
  twait { _he->call (_procno, &seqno, &res, mkevent (err)); }

  if (err) {
    warn << "clone RPC failed: " << err << "\n";
  } else if (res <= 0) {
    warn << "clone RPC: server failed to allocate socket\n";
  } else if (res != seqno) {
    warn << "clone RPC: server gave back wrong sequence number\n";
  } else {
    ret = _he->recvfd ();
    if (ret < 0) {
      warn << "clone RPC: received bad FD\n";
    }
  }
  ev->trigger (ret);
}
