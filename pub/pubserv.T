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
#include "pubd.h"

#ifdef HAVE_LINUX_PRCTL_DUMP
# include <sys/prctl.h>
#endif /* HAVE_LINUX_PRCTL_DUMP */

namespace pub3 {

  //-----------------------------------------------------------------------

  master_srv_t::master_srv_t (ptr<axprt_unix> x, ptr<local_publisher_t> p,
			      ptr<srv_file_lookup_t> lkp, int fdfd)
    : clone_server_t (x),
      _x (x),
      _srv (asrv::alloc (_x, pub_prog_3, wrap (this, &master_srv_t::dispatch))),
      _pub (p),
      _file_lookup (lkp),
      _personal_slave (new_slave (x, false))
  {}

  //-----------------------------------------------------------------------

  ptr<srv_file_lookup_t> slave_srv_t::file_lookup () 
  { return _master->file_lookup (); }

  //-----------------------------------------------------------------------

  ptr<local_publisher_t> slave_srv_t::pub () { return _master->pub (); }

  ptr<const local_publisher_t> slave_srv_t::pub () const 
  { return _master->pub (); }

  //-----------------------------------------------------------------------
  
  slave_srv_t::slave_srv_t (ptr<axprt_stream> x, master_srv_t *m, bool do_srv)
    : _x (x), 
      _cli (aclnt::alloc (_x, pub_prog_3)),
      _last_update (0),
      _push_deltas (false),
      _registered (true),
      _push_deltas_lock (false),
      _master (m)
  {
    if (do_srv) {
      _srv = asrv::alloc (_x, pub_prog_3, wrap (this, &slave_srv_t::dispatch));
    }
  }
 
  //-----------------------------------------------------------------------

  master_srv_t::~master_srv_t ()
  {
    if (_personal_slave) { delete_slave (_personal_slave); }
  }
  
  //-----------------------------------------------------------------------

  void slave_srv_t::handle_eof () { _master->delete_slave (this); }

  //-----------------------------------------------------------------------

  void master_srv_t::handle_eof () { _shutdown_ev->trigger (); }

  //-----------------------------------------------------------------------

  tamed void
  master_srv_t::run (evi_t ev)
  {
    twait { _shutdown_ev = mkevent (); }
    warn << "Caught EOF; shutting down.\n";
    ev->trigger (0);
  }

  //-----------------------------------------------------------------------

  void master_srv_t::handle_clonefd (svccb *sbp)
  {
    clone_server_t::clonefd (sbp);
  }
  
  //-----------------------------------------------------------------------

  void
  master_srv_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      handle_eof ();
    } else {
      u_int p = sbp->proc ();
      switch (p) {
      case PUB3_CLONE:
	handle_clonefd (sbp);
	break;
      default:
	if (_personal_slave) {
	  _personal_slave->dispatch (sbp);
	} else {
	  sbp->reject (PROC_UNAVAIL);
	  break;
	}
      }
    }
  }

  //-----------------------------------------------------------------------

  void
  slave_srv_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      handle_eof ();
      return;
    }

    u_int p = sbp->proc ();
    switch (p) {
    case PUB3_NULL:
      sbp->reply (NULL);
      break;
    case PUB3_GET_ROOT_CONFIG:
      config (sbp);
      break;
    case PUB3_GETFILE:
      getfile (sbp);
      break;
    case PUB3_GET_FSTATS:
      get_fstats (sbp);
      break;
    case PUB3_GET_PUSHES:
      { 
	bool f = file_lookup ()->do_pushes ();
	if (f)
	  _push_deltas = true;
	sbp->replyref (f);
      }
      break;
    case PUB3_GETCHUNK:
      getchunk (sbp);
      break;
    case PUB3_PUSH_DELTAS:
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

  //-----------------------------------------------------------------------

  void
  slave_srv_t::getchunk (svccb *sbp)
  {
    rpc::pub_prog_3::pub3_getchunk_srv_t<svccb> srv (sbp);

    xpub3_getchunk_arg_t *arg = srv.getarg ();
    xpub3_getchunk_res_t res;
    ptr<fhash_t> h = fhash_t::alloc (arg->hash);
    ptr<file_t> f = file_lookup ()->get_chunks (h, arg->opts);
    if (!f) {
      res.set_status (XPUB_STATUS_NOENT);
    } else {
      res.set_status (XPUB_STATUS_OK);
      res.chunk->data.setsize (arg->size);
      res.chunk->offset = arg->offset;
      ssize_t r = f->get_chunk (arg->offset, res.chunk->data.base (), 
				arg->size);
      if (r != ssize_t (arg->size)) {
	res.set_status (XPUB_STATUS_ERR) ;
	*res.error = "Datasize overflow";
      }
    }
    srv.reply (res);
  }
  
  //-----------------------------------------------------------------------

  tamed void
  slave_srv_t::getfile (svccb *sbp)
  {
    tvars {
      rpc::pub_prog_3::pub3_getfile_srv_t<svccb> srv (sbp);
      xpub3_getfile_arg_t *arg (srv.getarg ());
      ptr<file_t> f;
      xpub_status_t s;
      xpub3_getfile_res_t res (XPUB_STATUS_OK);
      u_int o;
      ssize_t sz;
    }
    o = arg->options;

    twait { 
      pub ()->getfile (arg->filename, mkevent (s,f), arg->fresh, o); 
    }

    if (!f) {
      assert (s.status != XPUB_STATUS_OK);
      res.set_code (s.status);
      *res.error_status = s;
    } else {

      // MK 2010/09/07 note -- there is a weird race condition here
      // when using file_lookup() of the chunkstore type.  If two 
      // processes request the same file at the same time, there will
      // be two insertions into the hold chunk cache, with slightly
      // different checksums (I believe in the metadata but I am not
      // sure).  As a result, one of the guys will successfully pass
      // a checksum on recovery, and the other giy won't.
      f->init_xdr_opaque ();
      sz = f->xdr_len ();
      assert (sz >= 0);
      if (sz > ssize_t (arg->maxsz)) {
	res.file->set_mode (XPUB_XFER_CHUNKED);
	res.file->chunked->datasize = sz;
	f->get_xdr_hash (&res.file->chunked->xdrhash);
	f->metadata ()->hash ().to_xdr (&res.file->chunked->dathash);
	res.file->chunked->leasetime = file_lookup ()->hold_chunks (f);
      } else {
	res.file->set_mode (XPUB_XFER_WHOLE);
	f->to_xdr (res.file->whole);
      }
    }
    srv.reply (res);
  }

  //-----------------------------------------------------------------------

  void
  master_srv_t::register_newclient (ptr<axprt_stream> x)
  {
    new_slave (x, true);
  }

  //-----------------------------------------------------------------------

  slave_srv_t *
  master_srv_t::new_slave (ptr<axprt_stream> x, bool do_srv)
  {
    slave_srv_t *s = New slave_srv_t (x, this, do_srv);
    _file_lookup->register_client (s);
    return s;
  }
  
  //-----------------------------------------------------------------------

  void
  master_srv_t::delete_slave (slave_srv_t *s)
  {
    _file_lookup->unregister_client (s);
    if (s == _personal_slave) { _personal_slave = NULL; }
    delete s;
  }

  //-----------------------------------------------------------------------


  bool
  master_srv_t::do_chroot (str dir, str uname, str gname) 
  {
    bool rc = false;
    ptr<jailer_t> j = pub ()->jailer ();
    if (getuid ()) {
      j->setjail (JAIL_VIRTUAL, dir);
      rc = true;
    } else {
      const char *dc = dir.cstr ();
      if (!uname) {
	uname = ok_pubd_uname;
	warn << "No pub username given; defaulting to '" <<  uname << "'\n";
      }
      if (!gname) {
	gname = ok_pubd_gname;
	warn << "No pub groupname given; defaulting to '" << gname << "'\n";
      }
      int uid = 0;
      int gid = 0;

      // need to call this before chroot'ing! otherwise, 
      // we can't open /etc/passwd
      if ((uid = uname2uid (uname)) < 0) {
	warn << uname << ": no such user\n";
      }
      if ((gid = gname2gid (gname)) < 0) {
	warn << gname << ": no such group\n";
      }

      if (uid < 0 || gid < 0) {
	warn ("cannot continue due to bad groups or users\n");
      } else if ((uid || gid) && setgroups (0, NULL)) {
	warn ("failed to void grouplist: %m\n");
      } else if (chroot (dc) != 0) {
	warn ("chroot (%s) failed: %m\n", dc);
      } else if (setgid (gid) != 0) {
	warn ("setgid (%s) failed: %m\n", gname.cstr ());
      } else if (setuid (uid) != 0) {
	warn ("setuid (%s) failed: %m\n", uname.cstr ());
      } else {
	j->setjail (JAIL_REAL, NULL);
	rc = true;
      }
    }

#ifdef HAVE_LINUX_PRCTL_DUMP
    if (prctl (PR_SET_DUMPABLE, 1) != 0) {
        warn ("prctl(PR_SET_DUMPABLE, 1) failed: %m\n");
    }
#endif /* HAVE_LINUX_PRCTL_DUMP */

    if (rc) j->set_in_jail (true);
    return rc;
  }

  //-----------------------------------------------------------------------

  str
  master_srv_t::jail2real (str s) const
  {
    str ret = pub ()->jailer ()->jail2real (s);
    return ret;
  }

  //-----------------------------------------------------------------------

};
