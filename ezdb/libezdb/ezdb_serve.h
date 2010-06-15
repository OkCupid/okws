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
#include "amt.h"
#include "ezdb_prot.h"
#include "ezdb_field.h"
#include "amysql.h"

namespace ezdb {
  
  //-----------------------------------------------------------------------
  
  class result_set_t {
  public:
    result_set_t (sth_id_t id, str q);
    bool load (sth_t sth, ezdb_error_pair_t *err);
    static result_set_t *alloc (sth_id_t id, str q, sth_t sth, 
				ezdb_error_pair_t *err);
    void add_field (const MYSQL_FIELD &f) { _fields.push_back (f); }
    vec<amysql_scalars_t> *results () { return &_results; }
    void reserve (size_t n) { _results.reserve (n); _n_rows = n; }
    void set_n_rows (size_t i) { _n_rows = i; }
    void fields_to_xdr (amysql_fields_t *out);
    bool in_bounds (size_t i) const { return i < _n_rows; }
    size_t next () { return _i++; }
    const amysql_scalars_t &get_row (size_t i) const { return _results[i]; }
    sth_id_t id () const { return _id; }
    str dump () const;
    void set_row_index (size_t i) { _i = i; }
    void set_insert_id (u_int64_t iid);
    size_t n_rows () const { return _n_rows; }
    u_int64_t insert_id () const { return _insert_id; }
    void set_expanded_query (str e) { _eq = e; }

    sth_id_t _id;
    str _q, _eq;
    vec<ezdb::field_t> _fields;
    vec<amysql_scalars_t> _results;
    size_t _n_rows;
    size_t _i;
    time_t _accessed;
    ihash_entry<result_set_t> _hlnk;
    tailq_entry<result_set_t> _qlnk;
    u_int64_t _insert_id;
  };
  
  //-----------------------------------------------------------------------

  class srv_t {
  public:
    srv_t (time_t timeout) ;
    void handle_fetch (const ezdb_fetch_arg_t *arg, ezdb_fetch_res_t *res);
    void handle_execute (amysql_thread_guts_t *thr,
			 const ezdb_execute_arg_t *arg,
			 ezdb_execute_res_t *res);
    void handle_finish (sth_id_t i, adb_status_t *res);
    void clean ();

  protected:
    void insert (result_set_t *r);
    result_set_t *get (sth_id_t i);
    bool remove (sth_id_t i);
    void remove (result_set_t *r);

  private:
    ihash<sth_id_t, result_set_t, &result_set_t::_id, 
	  &result_set_t::_hlnk> _results;
    tailq<result_set_t, &result_set_t::_qlnk> _q;
    qhash<str, str> _query_tab;
    u_int64_t _counter;
    time_t _timeout;
  };

  //-----------------------------------------------------------------------

};
