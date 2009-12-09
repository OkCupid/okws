
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
    xlate_mgr_t ();
    virtual ~xlate_mgr_t () {}

    void add_file (const xml_rpc_file &file);
    void add_files (const xml_rpc_file *const * list);

    void get_types(xml_req_t input, xml_resp_ev_t cb);
    void get_prog_info(xml_req_t input, xml_resp_ev_t cb);
    void xlate (xml_obj_const_t input, xml_resp_ev_t cb, CLOSURE);
    void get_constants (xml_req_t input, xml_resp_ev_t cb);

  protected:
    virtual void do_rpc (str hostname, int port, 
			 const rpc_program &prog,
			 int procno, const void *arg, void *res,
			 aclnt_cb cb)
    { do_rpc_T (hostname, port, prog, procno, arg, res, cb); }


    void add_const (const xml_rpc_const_t *c);
    void add_program (const xml_rpc_program *p);
    void add_type (const xml_typeinfo_t *t);

    qhash<str, const xml_rpc_program *> _programs;
    qhash<str, int> _constants;
    qhash<str, const xml_rpc_file *> _files;
    qhash<str, const xml_typeinfo_t *> _types;

    connmgr_t _cmgr;

  private:
    void do_rpc_T (str hostname, int port, 
		   const rpc_program &prog,
		   int procno, const void *arg, void *res,
		   aclnt_cb cb, CLOSURE);

    void fill_struct_info(xml_obj_ref_t x, const xml_struct_entry_t *entries,
                          int type, vec<str> &out, bhash<str> &seen);
    void fill_type_info(xml_obj_ref_t x, const xml_typeinfo_t *type,
                        vec<str> &out, bhash<str> &seen);
  };


  class xlate_retry_mgr_t : public xlate_mgr_t {
  public:
    xlate_retry_mgr_t ();
    void set_delays (const vec<timespec> &d) { _delays = d; }
  protected:
    void do_rpc (str hostname, int port, 
		 const rpc_program &prog,
		 int procno, const void *arg, void *res,
		 aclnt_cb cb)
    { do_rpc_T (hostname, port, prog, procno, arg, res, cb); }
  private:
    void do_rpc_T (str hostname, int port, 
		   const rpc_program &prog,
		   int procno, const void *arg, void *res,
		   aclnt_cb cb, CLOSURE);

    vec<timespec> _delays;
  };


};

#endif /* _LIBAHTTP_OKXMLXLATEMGR_H */
