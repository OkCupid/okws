/* $Id: pt1d.C 1007 2005-09-11 21:45:33Z max $ */

/*
 *
 * Copyright (C) 2003-4 by Maxwell Krohn (max@okcupid.com)
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

#include "ezdb_serve.h"

namespace ezdb {

  //-----------------------------------------------------------------------
  
  void
  srv_t::handle_fetch (const ezdb_fetch_arg_t *arg, ezdb_fetch_res_t *res)
  {
    result_set_t *rs = get (arg->sth_id);
    if (!rs) {
      res->set_status (ADB_BAD_FETCH);
    } else {
      size_t row = arg->row;
      if (rs->in_bounds (row)) {
	*res->row = rs->get_row (row);
	rs->set_row_index (row + 1);
      } else {
	res->set_status (ADB_NOT_FOUND);
      }
    }
  }
  
  //-----------------------------------------------------------------------
  
  void
  srv_t::handle_execute (amysql_thread_guts_t *thr,
			 const ezdb_execute_arg_t *arg,
			 ezdb_execute_res_t *res)
  {
    str q = arg->query;
    str loc = strbuf () << arg->code_location.file << ":" 
			<< arg->code_location.line;
    str eq = strbuf () << q << " --  @" << loc;

    sth_t sth;
    str *found;

    bool ok = true;

    if (arg->safe) {
      if (!(found = _query_tab[loc])) {
	_query_tab.insert (loc, q);
      } else if (*found != q) {
	res->set_status (ADB_SECURITY_FAILURE);
	str err = strbuf () << "Found query " << *found 
			    << " previous prepared for " << loc << "\n";
	res->error->desc = err;
	res->error->code = 0;
	ok = false;
      }
    } 

    if (!ok) { /* noop */ }
    else if (!(sth = thr->prepare (eq))) {
      adb_status_t code = thr->error_code ();
      if (!code) { code = ADB_BAD_QUERY; }
      res->set_status (code);
      res->error->desc = thr->error ();
      res->error->code = 0;
    } else if (!sth->execute_argvec (arg->args)) {
      res->set_status (ADB_BAD_QUERY);
      res->error->desc = sth->error ();
      res->error->code = sth->errnum ();
    } else {
      res->set_status (ADB_OK);
      ezdb_error_pair_t err;
      result_set_t *rs = result_set_t::alloc (_counter++, q, sth, &err);
      if (!rs) {
	res->set_status (ADB_BAD_FETCH);
	*res->error = err;
	delete rs;
      } else {
	rs->set_expanded_query (eq);
	res->res->sth_id = rs->id ();
	res->res->num_rows = rs->n_rows ();
	res->res->insert_id = rs->insert_id ();
	insert (rs);
	rs->fields_to_xdr (&res->res->fields);
      }
    }
  }
  
  //-----------------------------------------------------------------------
  
  void
  srv_t::handle_finish (sth_id_t id, adb_status_t *resp)
  {
    adb_status_t res = ADB_OK;
    if (get (id)) { remove (id); } 
    else { res = ADB_NOT_FOUND; }
    *resp = res;
  }
  
  //-----------------------------------------------------------------------
  
  void
  result_set_t::fields_to_xdr (amysql_fields_t *out)
  {
    out->setsize (_fields.size ());
    for (size_t i = 0; i < _fields.size (); i++) {
      _fields[i].to_xdr (&(*out)[i]);
    }
  }
  
  //-----------------------------------------------------------------------

  srv_t::srv_t (time_t t) : 
    _counter (sfs_get_timenow ()), 
    _timeout (t) {}

  //-----------------------------------------------------------------------
  
  bool 
  srv_t::remove (sth_id_t i)
  {
    result_set_t *res;
    bool found = false;
    if ((res = _results[i])) {
      remove (res);
      found = true;
    }
    return found;
  }

  //-----------------------------------------------------------------------
  
  void
  srv_t::remove (result_set_t *res)
  {
    _results.remove (res); 
    _q.remove (res);
    delete res;
  }
  
  //-----------------------------------------------------------------------
  
  result_set_t *
  srv_t::get (sth_id_t i)
  {
    result_set_t *res;
    if ((res = _results[i])) {
      res->_accessed = sfs_get_timenow ();
      _q.remove (res);
      _q.insert_tail (res);
    }
    return res;
  }
  
  //-----------------------------------------------------------------------
  
  void
  srv_t::insert (result_set_t *r)
  {
    clean ();
    _results.insert (r);
    _q.insert_tail (r);
  }
  
  //-----------------------------------------------------------------------

  result_set_t *
  result_set_t::alloc (sth_id_t id, str q, sth_t sth, ezdb_error_pair_t *err)
  {
    result_set_t *res = New result_set_t (id, q);
    size_t n = 0;
    const MYSQL_FIELD *fields = sth->fetch_fields (&n);
    if (fields) {
      for (size_t i = 0; fields && i < n; i++) {
	res->add_field (fields[i]);
      }
      adb_status_t status = ADB_OK;
      size_t nr = sth->n_rows ();
      res->reserve (nr);
      for (size_t i = 0; status == ADB_OK && i < nr; i++) {
	status = sth->fetch_argvec (res->results (), n);
      }
    } else {
      res->set_n_rows (sth->affected_rows ());
      res->set_insert_id (sth->insert_id ());
    }
    return res;
  }

  //-----------------------------------------------------------------------

  result_set_t::result_set_t (sth_id_t id, str q) : 
    _id (id),
    _q (q),
    _n_rows (0), 
    _i (0), 
    _accessed (sfs_get_timenow ()),
    _insert_id (0) {}
  

  //-----------------------------------------------------------------------

  void result_set_t::set_insert_id (u_int64_t iid) { _insert_id = iid; }

  //-----------------------------------------------------------------------

  void
  srv_t::clean ()
  {
    result_set_t *r;
    time_t n = sfs_get_timenow ();
    while ((r = _q.first) && (n - r->_accessed > _timeout)) {
      str q = r->dump ();
      warn << "Timing out stale query after " << (n - r->_accessed) << "s: " 
           << " (timeout = " << _timeout << "s): " << q << "\n";
      remove (r);
    }
  }

  //-----------------------------------------------------------------------

  str
  result_set_t::dump () const
  {
    strbuf b;
    str q = _eq ? _eq : _q;
    b << "[" << _i << "/" << _n_rows << "]: " << q;
    return b;
  }

  //-----------------------------------------------------------------------

};
  
  
