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

#include "pub.h"
#include "okdbg.h"

void
pub_rclient_t::publish (const xpub_fnset_t &files, pubrescb c)
{
  u_int sz = files.files.size ();
  vec<str> filesv;
  for (u_int i = 0; i < sz; i++)
    filesv.push_back (files.files[i]);
  publish (filesv, c);
}

void
pub_rclient_t::publish (const vec<str> &files, pubrescb c)
{
  u_int sz = files.size ();
  if (sz == 0) {
    (*c) (New refcounted<pub_res_t> ());
    return;
  }

  if (running) {
    waiters.push_back (New waiter_t (files, c));
    return;
  }
  res = New refcounted<pub_res_t> ();
  nset = init ? New pub_t::set_t () : set;
  cb = c;
  bool call = false;
  for (u_int i = 0; i < sz; i++) {

    //warn << "publish: " << files[i] << "\n"; //

    //
    // before we're initialized, we want to publish all files given to us;
    // otherwise, we're running due to an OKMGR_UPDATE RPC call -- in this
    // case, we need only explore files that are relevant to this service,
    // and their children in the pub tree
    //
    if (!init || set->bindings[files[i]]) {
      call = true;
      getfile (files[i]);
    }
  }

  if (!call) 
    (*c) (New refcounted<pub_res_t> ());
}

void
pub_rclient_t::getfile (const pfnm_t &fn)
{
  // warn << "getfile: " << fn << "\n"; // debug
  nreq++;
  lookup (fn, wrap (this, &pub_rclient_t::getfile_lookedup));
}

void
pub_rclient_t::lookup (const pfnm_t &fn, bindcb c)
{
  if (holdtab.inq (fn, c)) 
    return;
  xpub_fn_t x = fn;
  ptr<xpub_lookup_res_t> xr = New refcounted<xpub_lookup_res_t> ();
  clnt->call (lookup_mthd, &x, xr,
	      wrap (this, &pub_rclient_t::lookedup, fn, xr));
}

void
pub_rclient_t::lookedup (pfnm_t fn, ptr<xpub_lookup_res_t> xr,
			 clnt_stat err)
{
  pbinding_t *bnd = NULL;
  if (err) {
    res->add (err);
  } else if (xr->status != XPUB_STATUS_OK) {
    res->add_xdr_res (*xr, fn);
  } else {
    bnd = nset->alloc (fn, phash_t::alloc (*xr->hash));
  }
  holdtab.finish (fn, bnd);
}

void
pub_rclient_t::getfile_lookedup (pbinding_t *bnd)
{
  bool gfcall = false;
  pfile_t *f;
  --nreq;
  if (bnd && !nset->files[bnd->hash ()]) {
    if (init && (f = set->files[bnd->hash ()])) {

      // see comment below; order here is crucial, need to remove before
      // inserting, otherwise, we'll have very hard to find bugs.
      set->remove (f);
      nset->insert (f);
    } else {
      gfcall = true;
    }
  }
  if (gfcall) {
    getfile (bnd);
  } else if (!nreq)
    finish_publish ();
}

void
pub_rclient_t::getfile (pbinding_t *bnd)
{
  if (getfile_hold[bnd->hsh]) {
    assert (nreq > 0);
    return;
  }
  getfile_hold.insert (bnd->hsh);
  nreq++;
  xpubhash_t x;
  bnd->hsh->to_xdr (&x);
  ptr<xpub_getfile_res_t> xr = New refcounted<xpub_getfile_res_t> ();
  clnt->call (getfile_mthd, &x, xr, 
	      wrap (this, &pub_rclient_t::gotfile, xr, bnd));
}

void
pub_rclient_t::gotfile (ptr<xpub_getfile_res_t> xr, pbinding_t *bnd,
			clnt_stat err)
{
  --nreq;
  getfile_hold.remove (bnd->hsh);
  if (err) {
    res->add (err);
  } else if (xr->status != XPUB_STATUS_OK) {
    res->add_xdr_res (*xr, bnd->filename ());
  } else {
    pfile_t *f = New pfile_t (*xr->file);
    nset->insert (f);
    f->explore (EXPLORE_PARSE);
  }
  finish_req ();
}

void
pub_rclient_t::finish_req ()
{
  while (parsequeue.size ()) {
    pfnm_t nm = parsequeue.pop_front ();
    if (!nset->bindings[nm]) 
      getfile (nm);
  }

  if (!nreq)
    finish_publish ();
}

void
pub_rclient_t::finish_publish ()
{
  bool newtab = true;
  if (!init) {
    init = true;
    nset = NULL;
    run_configs ();
  } else if (*res && new_set ()) {
    delete set;
    set = nset;
    nset = NULL;
    run_configs ();
  } else {
    if (OKDBG2 (PUB_ERRORS)) {
      okdbg_warn (ERROR, "finish_publish did not install new binding set");
    }
    newtab = false;
    delete nset;
    nset = NULL;
  }

  if (newtab && OKDBG2 (PUB_BINDTAB_INSERTS)) {
    okdbg_warn (CHATTER, "PUB bind table installed:");
    set->bindings.okdbg_dump (CHATTER);
  }

  (*cb) (res);
  running = false;
  waiter_t *w;
  if (waiters.size ()) {
    w = waiters.pop_front ();
    publish (w->files, w->cb);
    delete w;
  }
}

bool
pub_rclient_t::new_set ()
{
  u_int lim = rootfiles.size ();
  for (u_int i = 0; i < lim; i++)
    explore (rootfiles[i]);
  return (*res);
}

void
pub_rclient_t::explore (const pfnm_t &fn) const
{
  bpfcp_t b = nset->getfile (fn);
  pfile_t *f = b ? b->file : NULL;
  if (!f) {
    const pbinding_t *b = set->bindings[fn];
    if (!b) {
      *res << (strbuf ("Cannot resolve file: ") << fn);
      return;
    }
    f = set->files[b->hash ()];
    if (f) { 
      // note that we should always remove before inserting; it's very
      // bad to have the same thing in two different hash tables, as their
      // hash link pointers will get crossed otherwise.
      set->remove (b, f);        
      nset->insert (b, f);
      
    } else {
      // maybe this file was already moved over by a previous walk down
      // the tree. if this is the case, then assert it's still here.
      assert (nset->files[b->hash ()]);

      // this insert (1) binds the filename given to the file's
      // current hash and (2) insert the file, key by its hash.
      // at this point, we need mapping (1) but not mapping (2)
      // since the mapping of hash to file is already there.
      // luckily, this function will do the right thing here.
      //
      nset->insert (b, f);
     
    }
  }
  // if file was previously explored, this f will be NULL, but
  // no need to reexplore.
  if (f)
    f->explore (EXPLORE_FNCALL);
}

pub_rclient_t *
pub_rclient_t::alloc (ptr<aclnt> c, int lm, int gm, int cm, u_int am, u_int om)
{
  parser = NULL;
  pub_rclient_t *rc = New pub_rclient_t (c, lm, gm, cm, am, om);
  pub = pcli = rc;
  pubincluder = rc;
  return rc;
}
