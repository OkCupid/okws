// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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
#include "pubd2.h"
#include "pubd.h"
#include "puberr.h"

namespace pubserv2 {

  srv_t::srv_t (ptr<axprt_stream> x, file_lookup_t *fl, bool p, int fdfd)
    : clone_server_t (fdfd),
      local_publisher_t (parser, 0, fl),
      _x (x), 
      _cli (aclnt::alloc (_x, pub_program_2)),
      _srv (asrv_delayed_eof::alloc (_x, pub_program_2, 
				     wrap (this, &srv_t::dispatch))),
      _primary (p),
      _last_update (0),
      _push_deltas (false),
      _registered (true),
      _push_deltas_lock (false),
      _file_lookup (fl)
  {
    _file_lookup->register_client (this);
    if (_primary && !clone_server_t::setup ())
      warn << "no socket setup for file descriptor passing!\n";
  }
  
  void
  srv_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      if (_primary) {
	warn << "EOF received; shutting down.\n";
	exit (0);
      } else {
	delete this;
	return;
      }
    }
    u_int p = sbp->proc ();
    switch (p) {
    case PUB_NULL:
      sbp->reply (NULL);
      break;
    case PUB2_GET_ROOT_CONFIG:
      config (sbp);
      break;
    case PUB2_GETFILE:
      getfile (sbp);
      break;
    case PUB2_GET_FSTATS:
      get_fstats (sbp);
      break;
    case PUB2_CLONE:
      // handled by base class: clone_server_t
      clonefd (sbp);
      break;
    case PUB2_GET_PUSHES:
      { 
	bool f = _file_lookup->do_pushes ();
	if (f)
	  _push_deltas = true;
	sbp->replyref (f);
      }
      break;
    case PUB2_PUSH_DELTAS:
      // Another Pub server (that local on the NFS server) might
      // be pushing deltas to US, which we should, turn around
      // and send to our clients.
      sbp->reject (PROC_UNAVAIL);
      break;
    default:
      sbp->reject (PROC_UNAVAIL);
      break;
    }
    return;
  }
  
  TAMED void
  srv_t::getfile (svccb *sbp)
  {
    VARS {
      xpub2_getfile_arg_t *arg (sbp->Xtmpl getarg<xpub2_getfile_arg_t> ());
      ptr<bound_pfile2_t> f;
      xpub_status_t s;
      xpub2_getfile_res_t res (XPUB_STATUS_OK);
      u_int o;
    }
    o = arg->options;

    BLOCK { 
      pub2::local_publisher_t::getfile (arg->filename, @(s,f), arg->fresh, o); 
    }

    if (!f) {
      assert (s.status != XPUB_STATUS_OK);
      res.set_status (s.status);
      if (s.status == XPUB_STATUS_ERR) {
	*res.error = *s.error;
      }
    } else {
      res.data->stat.ctime = f->ctime ();
      res.data->stat.fn = f->filename ();
      f->hash ()->to_xdr (&res.data->stat.hash);
      f->file ()->to_xdr (&res.data->file);
    }
    sbp->replyref (res);
  }

  void
  srv_t::register_newclient (ptr<axprt_stream> x)
  {
    client_accept (x);
  }
  
  srv_t::~srv_t ()
  {
    if (_registered) 
      _file_lookup->unregister_client (this);
  }
};
