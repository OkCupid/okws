// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Max Krohn (max@scs.cs.nyu.edu)
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

#ifndef _LIBPUB_AXPRT_FD_H
#define _LIBPUB_AXPRT_FD_H 1

#include "arpc.h"
#include "async.h"
#include "callback.h"

// send FD's down into the sink through this object. usually,
// an FD source has an FD sink object to send into.
class fdsink_t : public virtual refcount {
public:
  fdsink_t (ptr<axprt_unix> xx, cbv::ptr ec) 
    : ux (xx), eofcb (ec), eof (ux->ateof ()) 
  {
    ux->setrcb (wrap (this, &fdsink_t::rcb));
  }

  static ptr<fdsink_t> alloc (ptr<axprt_unix> xx, cbv::ptr ec)
  { return New refcounted<fdsink_t> (xx, ec); }

  // send the file descriptor, along with an RPC data packet.
  template<class T> bool send (int fd, const T &t)
  {
    if (eof)
      return false;

    xdrsuio x (XDR_ENCODE);
    XDR *xp = &x;
    if (!rpc_traverse (xp, const_cast<T &> (t)))
      return false;
    
    ux->sendfd (fd);
    ux->sendv (x.uio ()->iov (), x.uio ()->iovcnt ());
    
    return true;
  }

  void 
  rcb (const char *, ssize_t s, const sockaddr *)
  {
    if (s == 0 || ux->ateof ()) {
      eof = true;
      if (eofcb) {
	(*eofcb) ();
	eofcb = NULL;
      }
      ux->setrcb (NULL);
    } else if (s < 0) {
      if (errno != EAGAIN)
	warn ("read error in fdsink_t: %m\n");
    } else {
      warn << "data sent to fdsink_t ignored (" << s << " bytes)\n";
    }
  }

  ptr<axprt_unix> ux;
  cbv::ptr eofcb;
  bool eof;
};

template<typename T>
class fdsource_t : public virtual refcount {
public:
  fdsource_t (int f, 
	      typename callback<void, int, ptr<T> >::ptr c, 
	      size_t ps = axprt_stream::defps) 
    :  x (axprt_unix::alloc (f, ps)), cb (c), fd (f)
  {
    x->setrcb (wrap (this, &fdsource_t<T>::recv_cb));
  }

  static ptr<fdsource_t<T> > 
  alloc (int f, typename callback<void, int, ptr<T> >::ptr c)
  { 
    if (!isunixsocket (f))
      return NULL;
    else
      return New refcounted<fdsource_t<T> > (f, c); 
  }
  
private:

  void recv_cb (const char *pkt, ssize_t len, const sockaddr *)
  {
    int fd;
    if (pkt && (fd = x->recvfd ()) >= 0 && len > 0) {
      xdrmem xx (pkt, len);
      XDR *xp = &xx;
      ptr<T> t = New refcounted<T> ();
      if (rpc_traverse (xp, *t)) {
	(*cb) (fd, t);
	return;
      } else {
	warn << "rpc_traverse failed\n";
	close (fd);
	return;
      }
    }
    // send EOF signal to listener
    x->setrcb (NULL);
    (*cb) (-1, NULL);
  }

  ref<axprt_unix> x;
  typename callback<void, int, ptr<T> >::ptr cb;
  int fd;
};

#endif /* _LIBPUB_AXPRT_FD_H */
