
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

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

#ifndef _LIBAHTTP_OKXMLXLATEMGR_H
#define _LIBAHTTP_OKXMLXLATEMGR_H

//
// Class for managing translation, connections, etc.
//

#include "async.h"
#include "arpc.h"
#include "tame.h"
#include "ihash.h"
#include "list.h"
#include "tame_lock.h"
#include "okxmlxlate.h"
#include "okxml.h"
#include "okxmlobj.h"

namespace okxml {

  typedef callback<void, ptr<axprt> >::ref xcb_t;

  str to_netloc_hash (const str &h, int p);


  //
  // XXX: todo: cache aclnt objects along with transports.
  //
  struct conn_t : public virtual refcount {

    conn_t (const str &h, int p);

    void connect (cbb cb, CLOSURE);
    void touch ();
    void release ();
    bool connected () const;
    ptr<axprt> x () { return _x; }

    const str _hostname;
    const int _port;
    const str _hashkey;

    const time_t _created;
    time_t _accessed;

    ptr<axprt_stream> _x;
    ihash_entry<conn_t> _hlnk;
    tailq_entry<conn_t> _qlnk;

    tame::lock_t _lock;
    ptr<conn_t> _self;
  };

  class connmgr_t {
  public:
    connmgr_t () {}

    void getcon (const str &h, int p, xcb_t cb, CLOSURE);

  private:
    ihash<const str, conn_t, &conn_t::_hashkey, &conn_t::_hlnk> _tab;
    tailq<conn_t, &conn_t::_qlnk> _q;
  };

  class xlate_mgr_t {
  public:
    xlate_mgr_t () {}

    void add_file (const xml_rpc_file &file);

    void xlate (xml_obj_const_t input,
		xml_obj_t *output,
		aclnt_cb cb, CLOSURE);

  private:
    void add_const (const xml_rpc_const_t *c);
    void add_program (const xml_rpc_program *p);

    qhash<str, const xml_rpc_program *> _programs;
    qhash<str, int> _constants;
    qhash<str, const xml_rpc_file *> _files;

    connmgr_t _cmgr;
  };

};

#endif /* _LIBAHTTP_OKXMLXLATEMGR_H */
