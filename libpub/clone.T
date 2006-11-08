// -*-c++-*-
/* $Id$ */

#include "okclone.h"
#include "okdbg.h"
#include "tame.h"

void
clone_server_t::eofcb ()
{
  warn << "EOF on FD channel; sink is stopped up\n";
  _fdsnk = NULL;
}

bool
clone_server_t::setup ()
{
  if (_fdfd < 0) {
    warn << "no socket setup for file descriptor passing!\n";
    return false;
  }
  _fdsnk = fdsink_t::alloc (axprt_unix::alloc (_fdfd, ok_axprt_ps),
			    wrap (this, &clone_server_t::eofcb));
  return true;
}

void
clone_server_t::clonefd (svccb *b)
{
  if (!_fdsnk) {
    warn << "FD requested after sink was stopped up!\n";
    b->replyref (false);
    return;
  }

  bool ret = true;
  int fds[2];
  int rc = socketpair (AF_UNIX, SOCK_STREAM, 0, fds);
  if (rc < 0) {
    warn ("socketpair: %m\n");
    ret = false;
  } else {
    register_newclient (axprt_unix::alloc (fds[0], ok_axprt_ps));
    _fdsnk->send (fds[1], _fdseqno++);
  }
  b->replyref (ret);
}

void
clone_server_t::close ()
{
  if (_fdfd >= 0) 
    ::close (_fdfd);
}


bool
clone_client_t::init ()
{
  int fd = _he->get_sock ();
  if (OKDBG2(OKLD_FD_PASSING)) {
    strbuf b;
    b << "using FD=" << fd << " for receiving passed FDs\n";
    okdbg_warn (CHATTER, b);
  }

  if (fd < 0)
    return false;
  _fdsrc = fdsource_t<u_int32_t>::alloc (fd, wrap (this, 
						   &clone_client_t::gotfd));
  return true;
}
  
void
clone_client_t::gotfd (int nfd, ptr<u_int32_t> id)
{
  if (nfd >= 0) {
    _fds.push_back (nfd);
  }

  // 'if' and not 'while', since the callback should always be there before
  if (_cbq.size ()) {
    cbi cb = _cbq.pop_front ();
    nfd = _fds.size () ? _fds.pop_front () : -1;
    (*cb) (nfd);
  }
}

tamed void 
clone_client_t::clone (cbi cb)
{
  tvars {
    bool b;
    clnt_stat err;
  }

  twait { _he->call (_procno, NULL, &b, mkevent (err)); }

  if (err) {
    warn << "clone RPC failed: " << err << "\n";
    (*cb) (-1);
  } else if (!b) {
    warn << "clone server returned failure on clone request.\n";
    (*cb) (-1);
  } else if (_fds.size ()) {
    (*cb) (_fds.pop_front ());
  } else {
    _cbq.push_back (cb);
  }
}
