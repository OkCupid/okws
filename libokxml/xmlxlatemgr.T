
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxmlxlatemgr.h"
#include "okconst.h"

namespace okxml {

  str
  to_netloc_hash (const str &h, int p)
  {
    return strbuf ("%s:%d", h.cstr (), p);
  }

  conn_t::conn_t (const str &h, int p)
    : _hostname (h),
      _port (p),
      _hashkey (to_netloc_hash (h, p)),
      _created (sfs_get_timenow ()),
      _self (mkref (this))
  {}

  void
  conn_t::touch ()
  {
    _accessed = sfs_get_timenow ();
  }

  void
  conn_t::release ()
  {
    _self = NULL;
  }

  tamed void 
  conn_t::connect (cbb cb)
  {
    tvars {
      int fd;
      bool ret (false);
    }
    twait { tcpconnect (_hostname, _port, mkevent (fd)); }
    if (fd >= 0) {
      _x = axprt_stream::alloc (fd, ok_axprt_ps);
      ret = true;
    }
    (*cb) (ret);
  }

  bool
  conn_t::connected () const
  {
    return (_x && !_x->ateof ());
  }

  tamed void
  connmgr_t::getcon (const str &h, int p, xcb_t cb)
  {
    tvars {
      conn_t *c;
      ptr<conn_t> ch;
      bool ok;
      bool do_release (false);
      ptr<axprt> x;
    }
    c = _tab[to_netloc_hash (h, p)];
    if (c) {
      ch = mkref (c);
      _q.remove (c);
      _q.insert_tail (c);
      c->touch ();
    } else {
      ch = New refcounted<conn_t> (h, p);
      c = ch;
      _tab.insert (c);
      _q.insert_tail (c);
    }

    if (!c->connected ()) {
      twait { c->_lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }
      twait { c->connect (mkevent (ok)); }
      do_release = true;
    } else {
      ok = true;
    }

    if (ok) {
      x = c->x ();
    }

    (*cb)(x);

    if (do_release)
      c->_lock.release ();
  }

  void
  xlate_mgr_t::add_program (const xml_rpc_program *p)
  {
    _programs.insert (p->name, p);
  }

  void
  xlate_mgr_t::add_const (const xml_rpc_const_t *c)
  {
    _constants.insert (c->name, c->val);
  }

  void
  xlate_mgr_t::add_file (const xml_rpc_file *file)
  {
    _files.insert (file->filename, file);
    for (const xml_rpc_program **p = file->programs; *p; p++) {
      add_program (*p);
    }
    for (const xml_rpc_const_t *c = file->constants; c->name; c++) {
      add_const (c);
    }
  }
}

