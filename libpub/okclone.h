// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Max
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

#ifndef _LIBPUB_OKCLONE_H
#define _LIBPUB_OKCLONE_H

#include "arpc.h"
#include "pslave.h"
#include "tame.h"

/**
 * A server that is able to clone FDs to itself, to allow
 * new clients to connect over Unix FD passing.  Can be plugged into
 * RPC servers that have a RPC procedure of the form:
 *
 *   int XXX_CLONE_FD_XXX (int) = 5;
 *
 * Will send back the input on success, or -1 on failure.
 */
class clone_server_t {
public:
  virtual ~clone_server_t () {}
  clone_server_t (ptr<axprt_unix> x) : _x (x) {}
  void clonefd (svccb *sbp);
  virtual void register_newclient (ptr<axprt_stream> cli) = 0;
private:
  ptr<axprt_unix> _x;
};

/**
 * A client class for getting cloned FDs from a cloned server.
 */
class clone_client_t {
public:
  clone_client_t (helper_exec_t *h, int p) : _he (h), _procno (p), _seqno (1) {}
  void clone (evi_t ev, CLOSURE);
protected:
  helper_exec_t *_he;
  int _procno;
  int32_t _seqno;
};

/**
 * A client that only connects to a remote resource, for cloning purposes
 */
class clone_only_client_t : public clone_client_t {
public:
  clone_only_client_t (helper_exec_t *h, int p) : clone_client_t (h, p) {}
  void connect (cbb cb) { _he->connect (cb); }
};

#endif /* _LIBPUB_OKCLONE_H */
