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

class axprt_fd_t : public axprt_unix {
public:
  axprt_fd_t (int f, size_t ps, size_t bs = 0)
    : axprt_unix (f, ps, bs) {}

  static ptr<axprt_fd_t> alloc (int f, size_t ps = axprt_stream::defps)
  { return New refcounted<axprt_fd_t> (f, ps); }

  // send the file descriptor, along with an RPC data packet.
  template<class T> bool clone (int fd, const T &t)
  {
    xdrsuio x (XDR_ENCODE);
    XDR *xp = &x;
    if (!rpc_traverse (xp, const_cast<T &> (t)))
      return false;
    
    sendfd (fd);
    sendv (x.uio ()->iov (), x.uio ()->iovcnt ());
    
    return true;
  }

};

template<typename T>
class fdsink_t {
public:
  fdsink_t (int f, 
	    typename callback<void, int, ptr<T> >::ptr c, 
	    size_t ps = axprt_stream::defps) 
    :  x (axprt_unix::alloc (f, ps)), cb (c), fd (f)
  {
    x->setrcb (wrap (this, &fdsink_t::recv_cb));
  }
  
private:

  void recv_cb (const char *pkt, ssize_t len, const sockaddr *)
  {
    if (pkt) {
      fd = x->recvfd ();
      if (fd) {
	xdrmem xx (pkt, len);
	XDR *xp = &xx;
	ptr<T> t = New refcounted<T> ();
	if (rpc_traverse (xp, *t)) {
	  (*cb) (fd, t);
	  return;
	}
      } else 
	x->setrcb (NULL);
    }
    (*cb) (-1, NULL);
  }

  ref<axprt_unix> x;
  typename callback<void, int, ptr<T> >::ptr cb;
  int fd;
};

#endif /* _LIBPUB_AXPRT_FD_H */
