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

bool
pub_proxy_t::lookup (const xpub_fn_t &x, xpub_lookup_res_t *res)
{
  pfnm_t nm = x;
  pbinding_t *bnd;
  if ((bnd = bindings[nm])) {
    res->set_status (XPUB_STATUS_OK);
    bnd->hash ()->to_xdr (res->hash);
    return true;
  } else {
    res->set_status (XPUB_STATUS_NOENT);
    return false;
  }
}

bpfcp_t
pub_proxy_t::v_getfile (const pfnm_t &nm) const 
{
  const bpfcp_t *rp = my_cache[nm];
  if (rp) return *rp;

  bpfcp_t ret;
  pbinding_t *bnd;
  if ((bnd = bindings[nm])) {
    const ptr<xpub_file_t> *f = files[bnd->hsh];
    if (!f)
      return NULL;

    // 4th arg is true -> delete file when done
    ret = bound_pfile_t::alloc (bnd, New pfile_t (**f), NULL, true);
    my_cache.insert (nm, ret);
    return ret;
  } else {
    return NULL;
  }
}

bool
pub_proxy_t::getfile (const phashp_t &hsh, xpub_getfile_res_t *res)
{
  ptr<xpub_file_t> *f = files[hsh];
  if (f) {
    res->set_status (XPUB_STATUS_OK);
    *res->file = **f;
    return true;
  } else {
    res->set_status (XPUB_STATUS_ERR);
    return false;
  }
}

void
pub_proxy_t::clear ()
{
  bindings.clear2 ();
  files.clear ();
}

void
pub_proxy_t::cache (const xpub_set_t &st)
{
  // cheap enough to refill...
  my_cache.clear ();

  for (u_int i = 0; i < st.bindings.size (); i++) {
    const xpub_pbinding_t &b = st.bindings[i];
    cache (b);
  }
  for (u_int i = 0; i < st.files.size (); i++) {
    const xpub_file_t &f = st.files[i];
    cache (f);
  }
  recycle.clear ();
}

void
pub_proxy_t::cache (const pfnm_t &fn, phashp_t hsh)
{
  pbinding_t *bnd = New pbinding_t (fn, hsh);
  bindings.bind (bnd);
}

void
pub_proxy_t::cache (const xpub_pbinding_t &bnd)
{
  pbinding_t *b = New pbinding_t (bnd);
  bindings.bind (b);
  phashp_t hsh = b->hash ();
  ptr<xpub_file_t> *fp = recycle[hsh];
  ptr<xpub_file_t> f;
  if (fp) {
    f = *fp;
    recycle.remove (hsh);
    files.insert (hsh, f);
  }
}

void
pub_proxy_t::cache (const xpub_file_t &f)
{
  phashp_t hsh = phash_t::alloc (f.hsh);
  bool do_insert = true;
  if (!files[hsh]) 
    files.insert (hsh, New refcounted<xpub_file_t> (f));
}

void
pub_proxy_t::cache (ptr<xpub_file_t> f)
{
  phashp_t hsh = phash_t::alloc (f->hsh);
  if (!files[hsh]) 
    files.insert (hsh, f);
}

void
pub_proxy_t::remove (phashp_t hsh)
{
  ptr<xpub_file_t> *f = files[hsh];
  if (f) {
    recycle.insert (hsh, *f);
    files.remove (hsh);
  }
}

void
pub_proxy_t::cache_pubconf (const xpub_file_t &x)
{
  pconf = New refcounted<xpub_file_t> (x);
}
