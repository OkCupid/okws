// -*-c++-*-

#include "ezdb.h"
#include "ezdb_prot.h"

namespace ezdb {

  //-----------------------------------------------------------------------

  cli_t::cli_t (const str &hn, int port, u_int opts)
    : _con (New helper_inet_t (ezdb_prog_1, hn, port, opts)),
      _del_con (true) {}

  //-----------------------------------------------------------------------

  cli_t::cli_t (helper_inet_t *c, bool dc) : _con (c), _del_con (dc) {}

  //-----------------------------------------------------------------------

  cli_t::~cli_t () { if (_del_con && _con) delete _con; }

  //-----------------------------------------------------------------------

  tamed void
  sth_base_t::fetch_bundle (ev_t ev, binder::from_xdr_bundle_t res)
  {
    tvars {
      amysql_scalars_t a;
      str s;
      adb_status_t c;
    }
    twait { fetch_xdr_union (mkevent (c, s), &a); }
    if (c != ADB_OK) { /* noop */ }
    else if (a.size () < res.size ()) {
      c = ADB_PARAM_OVERRUN;
      s = strbuf ("Too many params; wanted %zu but only %zu returned",
		  res.size (), a.size ());

    } else for (size_t i = 0; i < res.size (); i++) {
	res[i].from_xdr (a[i]);
    }
    ev->trigger (c, s);
  }

  //-----------------------------------------------------------------------

  sth_base_t::sth_base_t (ptr<cli_t> c, str s, safe_t safe, str f, int l) 
    : _cli (c), 
      _query (s),
      _safe (safe),
      _file (f),
      _line (l),
      _id (0),
      _alive (false),
      _num_rows (0),
      _iter (0),
      _insert_id (0) {}

  //-----------------------------------------------------------------------

  sth_base_t::~sth_base_t () {}

  //-----------------------------------------------------------------------

  tamed void
  sth_base_t::execute_bundle (ev_t ev, binder::to_xdr_bundle_t b)
  {
    tvars {
      amysql_scalars_t args;
      str s;
      adb_status_t c;
      size_t i;
    }
    
    args.setsize (b.size ());
    for (i = 0; i < b.size (); i++) { args[i] = b[i].to_xdr (); }
    twait { execute_xdr_union (mkevent (c, s), args); }
    ev->trigger (c, s);
  }

  //-----------------------------------------------------------------------

  tamed void 
  sth_base_t::fetch_xdr_union (ev_t ev, amysql_scalars_t *row)
  {
    tvars {
      ezdb_fetch_res_t res;
      ezdb_fetch_arg_t arg;
      adb_status_t c;
      str s;
      clnt_stat err;
    }

    if (!_alive || !_id) {
      c = ADB_DEAD_QUERY;
      s = "The current query is dead; can't fetch on it";
    } else if (_iter >= _num_rows) {
      c = ADB_NOT_FOUND;
      s = "End of result set";
    } else {
      arg.sth_id = _id;
      arg.row = _iter++;
      twait { 
	rpc::ezdb_prog_1::ezdb_fetch (_cli->con(), arg, &res, 
				      mkevent (err)); 
      }
      if (err) {
	s = strbuf () << err;
	c = ADB_RPC_ERROR;
      } else if (res.status != ADB_OK) {
	c = res.status;
	s = res.error->desc;
      } else {
	c = ADB_OK;
	*row = *res.row;
      }
    }
    ev->trigger (c, s);
  }

  //-----------------------------------------------------------------------

  tamed void 
  sth_base_t::execute_xdr_union (ev_t ev, const amysql_scalars_t &args)
  {
    tvars {
      ezdb_execute_arg_t arg;
      ezdb_execute_res_t res;
      clnt_stat err;
      adb_status_t c;
      str s;
    }

    // Finish any previous queries issues on the STH
    finish ();

    if (!_file || !_line) {
      c = ADB_BAD_PREPARE;
      s = "need a file name & line number when preparing a query";
    } else {
      arg.safe = (_safe == SAFE ? TRUE : FALSE);
      arg.code_location.file = _file;
      arg.code_location.line = _line;
      arg.query = _query;
      arg.args = args;
      
      twait { 
	rpc::ezdb_prog_1::ezdb_execute (_cli->con(), arg, &res, mkevent (err)); 
      }
      
      if (err) {
	s = strbuf () << err;
	c = ADB_RPC_ERROR;
      } else if (res.status != ADB_OK) {
	c = res.status;
	s = res.error->desc;
      } else {
	c = ADB_OK;
	_alive = true;
	_id = res.res->sth_id;
	_num_rows = res.res->num_rows;
	_fields.init (res.res->fields);
	_insert_id = res.res->insert_id;
	_iter = 0;
      }
    }

    ev->trigger (c, s);
  }
  
  //-----------------------------------------------------------------------

  tamed void
  sth_base_t::finish (ev_t::ptr ev)
  {
    tvars {
      sth_id_t tmp;
      adb_status_t c;
      str s;
      clnt_stat err;
    }

    if (_alive && _id) {
      _alive = false;
      tmp = _id;
      _id = 0;

      twait { 
	rpc::ezdb_prog_1::ezdb_finish (_cli->con(), tmp, &c, mkevent (err)); 
      }
      if (err) {
	s = strbuf () << err;
	c = ADB_RPC_ERROR;
      }
    }

    if (ev) ev->trigger (c, s);
  }

  //-----------------------------------------------------------------------

  void sth_base_t::finalize () 
  {
    finish ();
    delete this;
  }

  //-----------------------------------------------------------------------

  ptr<sth_t>
  cli_t::prepare (str s, safe_t safe, str file, int line) 
  {
    ptr<sth_t> out = New refcounted<sth_t> (mkref (this), s, safe, file, line);
    return out;
  }

  //-----------------------------------------------------------------------

  tamed void
  cli_t::connect (evb_t ev)
  {
    tvars {
      bool ok;
    }
    twait { _con->connect (mkevent (ok)); }
    ev->trigger (ok);
  }
  
  //-----------------------------------------------------------------------
};
