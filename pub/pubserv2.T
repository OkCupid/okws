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
#include "puberr.h"

namespace pubserv2 {

  primary_srv_t::primary_srv_t (ptr<axprt_stream> x, file_lookup_t *fl,
				int fdfd)
    : clone_server_t (fdfd), srv_t (x, fl)
  {
    if (!clone_server_t::setup ()) {
      warn << "no socket setup for file descriptor passing!\n";
    }
  }

  srv_t::srv_t (ptr<axprt_stream> x, file_lookup_t *fl)
    : local_publisher_t (global_parser, 0, fl),
      _x (x), 
      _cli (aclnt::alloc (_x, pub_program_2)),
      _srv (asrv_delayed_eof::alloc (_x, pub_program_2, 
				     wrap (this, &srv_t::dispatch))),
      _last_update (0),
      _push_deltas (false),
      _registered (true),
      _push_deltas_lock (false),
      _file_lookup (fl)
  {
    _file_lookup->register_client (this);
  }

  void srv_t::handle_eof () { delete this; }

  void primary_srv_t::handle_eof ()
  {
    warn << "Caught EOF; shutting down.\n";
    delete this;
    exit (0);
  }

  void srv_t::handle_clonefd (svccb *sbp)
  {
    sbp->reject (PROC_UNAVAIL);
  }

  void primary_srv_t::handle_clonefd (svccb *sbp)
  {
    // Handled by base class clone_server_t
    clonefd (sbp);
  }
  
  void
  srv_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      handle_eof ();
      return;
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
      handle_clonefd (sbp);
      break;
    case PUB2_GET_PUSHES:
      { 
	bool f = _file_lookup->do_pushes ();
	if (f)
	  _push_deltas = true;
	sbp->replyref (f);
      }
      break;
    case PUB2_GETCHUNK:
      getchunk (sbp);
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

  void
  srv_t::getchunk (svccb *sbp)
  {
    RPC::pub_program_2::pub2_getchunk_srv_t<svccb> srv (sbp);
    xpub2_getchunk_arg_t *arg = srv.getarg ();
    xpub2_getchunk_res_t res;
    phashp_t h = phash_t::alloc (arg->hash);
    ptr<bound_pfile2_t> f = _lookup->get_chunks (h, arg->opts);
    if (!f) {
      res.set_status (XPUB_STATUS_NOENT);
    } else {
      res.set_status (XPUB_STATUS_OK);
      res.chunk->data.setsize (arg->size);
      res.chunk->offset = arg->offset;
      ssize_t r = f->file ()->get_chunk (arg->offset, res.chunk->data.base (),
					 arg->size);
      res.chunk->offset = arg->offset;
      if (r != ssize_t (arg->size)) {
	res.set_status (XPUB_STATUS_ERR) ;
	*res.error = "Datasize overflow";
      }
    }
    srv.reply (res);
  }
  
  tamed void
  srv_t::getfile (svccb *sbp)
  {
    tvars {
      RPC::pub_program_2::pub2_getfile_srv_t<svccb> srv (sbp);
      xpub2_getfile_arg_t *arg (srv.getarg ());
      ptr<bound_pfile2_t> f;
      xpub_status_t s;
      xpub2_getfile_res_t res (XPUB_STATUS_OK);
      u_int o;
      ssize_t sz;
    }
    o = arg->options;

    twait { 
      pub2::local_publisher_t::getfile (arg->filename, mkevent (s,f), 
					arg->fresh, o); 
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

      f->file ()->init_xdr_opaque ();
      sz = f->file ()->len ();
      assert (sz >= 0);
      if (sz > ssize_t (arg->maxsz)) {
	res.data->file.set_mode (XPUB_XFER_CHUNKED);
	res.data->file.chunked->datasize = sz;
	f->file ()->get_xdr_hash (&res.data->file.chunked->xdrhash);
	res.data->file.chunked->leasetime = _lookup->hold_chunks (f);
      } else {
	res.data->file.set_mode (XPUB_XFER_WHOLE);
	f->file ()->to_xdr (res.data->file.whole);
      }
    }
    srv.reply (res);
  }

  void
  primary_srv_t::register_newclient (ptr<axprt_stream> x)
  {
    vNew srv_t (x, _file_lookup);
  }
  
  srv_t::~srv_t ()
  {
    if (_registered) {
      _file_lookup->unregister_client (this);
    }
  }

};
