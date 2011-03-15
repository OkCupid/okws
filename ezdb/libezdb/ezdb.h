// -*-c++-*-
/* $Id: ok.h 5189 2010-03-23 21:47:11Z max $ */

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

#pragma once

#include "async.h"
#include "arpc.h"
#include "ezdb_prot.h"
#include "tame.h"
#include "pslave.h"
#include "ezdb_field.h"
#include "ezdb_bind.h"
#include "amysql.h"

namespace ezdb {

  //-----------------------------------------------------------------------

  typedef enum { SAFE = 0, UNSAFE = 1 } safe_t;

  //-----------------------------------------------------------------------

  class cli_t;

  //-----------------------------------------------------------------------

  typedef event<adb_status_t, str>::ref ev_t;

  //-----------------------------------------------------------------------

  class sth_base_t : public virtual refcount {
  public:
    sth_base_t (ptr<cli_t> c, str q, safe_t safe, str file, int line);
    ~sth_base_t ();
    void execute_xdr_union (ev_t ev, const amysql_scalars_t &args, CLOSURE);
    void execute_bundle (ev_t ev, binder::to_xdr_bundle_t b, CLOSURE);
    void fetch_xdr_union (ev_t ev, amysql_scalars_t *row, CLOSURE);
    void fetch_bundle (ev_t ev, binder::from_xdr_bundle_t b, CLOSURE);
    void finish (ev_t::ptr ev = NULL, CLOSURE);
    void finalize (); // have a chance to finish queries
    u_int64_t insert_id () const { return _insert_id; }

  private:

    ptr<cli_t> _cli;
    str _query;
    safe_t _safe;
    str _file;
    int _line;
    sth_id_t _id;
    bool _alive;
    size_t _num_rows;
    size_t _iter;
    u_int64_t _insert_id;

    fields_t _fields;
  };

  //-----------------------------------------------------------------------

  class sth_t : public sth_base_t {
  public:
    sth_t (ptr<cli_t> c, str q, safe_t s, str f, int l) 
      : sth_base_t (c, q, s, f, l) {}

    // Read in auto-generated stubs that compose sth_t from here:
#include "ezdb_ag.h"

  };

  //-----------------------------------------------------------------------

  class cli_t : public virtual refcount {
  public:
    cli_t (const str &hn, int port = EZDBD_PORT, u_int opts = 0);
    cli_t (helper_inet_t *c, bool del_con = false);
    ~cli_t ();
    ptr<sth_t> prepare (str s, safe_t safe, str loc, int ln);
    helper_inet_t *con () { return _con; }

    void connect (evb_t ev, CLOSURE);

#define EZ_PREPARE(s)				\
    prepare (s, ezdb::SAFE, __FILE__, __LINE__)
#define UNSAFE_EZ_PREPARE(s)			\
    prepare (s, ezdb::UNSAFE, __FILE__, __LINE__)
  private:
    helper_inet_t *_con;
    bool _del_con;
  };

  //-----------------------------------------------------------------------
};
